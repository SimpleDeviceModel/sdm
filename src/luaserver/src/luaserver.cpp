/*
 * Copyright (c) 2015-2021 by Microproject LLC
 * 
 * This file is part of the Simple Device Model (SDM) framework.
 * 
 * SDM framework is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SDM framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SDM framework.  If not, see <https://www.gnu.org/licenses/>.
 *
 * LuaServer is a small wrapper library integrating Lua interpreter
 * into a C++ program.
 *
 * This module implements the main LuaServer class.
 */

#include "luaserver.h"
#include "luacallbackobject.h"
#include "luaiterator.h"

#include "stringutils.h"

#include <stdexcept>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cerrno>

/***************************************
 * LuaServer public interface
 ***************************************/

// Constructors and destructor

LuaServer::LuaServer(): _ownState(true) {
	_lua=luaL_newstate();
	if(!_lua) throw std::runtime_error("Cannot create Lua state");
	luaL_openlibs(_lua);

// Write pointer to self to the Lua registry (for the hook function)
	lua_pushstring(_lua,"SDMLuaServerPointer");
	lua_pushlightuserdata(_lua,this);
	lua_settable(_lua,LUA_REGISTRYINDEX);

// Set up termination hook
	lua_sethook(_lua,hookStackGuard,LUA_MASKCOUNT,5000);
}

// Attach LuaServer to the already existing state

LuaServer::LuaServer(lua_State *L): _lua(L),_ownState(false) {}

LuaServer::~LuaServer() {
	auto lock=_taskCV.getLock();
	_finishWorkerThread=true;
	_taskCV.notify();
	lock.unlock();
	if(_thread.joinable()) _thread.join();
	_reg.clear(); // destroy managed objects
	if(_ownState) lua_close(_lua); // close Lua state
}

// Member functions related to LuaServer confuguration

void LuaServer::attach(lua_State *L) {
	if(_ownState&&_lua) lua_close(_lua);
	_lua=L;
	_ownState=false;
}

// Member functions related to job start and termination

LuaCallResult LuaServer::executeChunk(const std::string &strChunk,const std::string &strName) {
	LuaCallResult res;
	res=loadChunk(strChunk,strName);
	if(!res.success) return res;
	return execute();
}

LuaCallResult LuaServer::executeChunk(LuaStreamReader &reader,const std::string &strName) {
	LuaCallResult res;
	res=loadChunk(reader,strName);
	if(!res.success) return res;
	return execute();
}

void LuaServer::executeChunkAsync(const std::string &strChunk,const std::string &strName,const Completer &completer) {
	if(_running) throw std::runtime_error("Lua interpreter is busy");
	
	LuaCallResult res;
	res=loadChunk(strChunk,strName);
	if(!res.success) return completer(res);
	
	_running=true;
	
	auto lock=_taskCV.getLock();
	_task=completer;
	if(!_thread.joinable()) _thread=std::thread(&LuaServer::threadProc,this);
	else _taskCV.notify();
}

void LuaServer::executeChunkAsync(LuaStreamReader &reader,const std::string &strName,const Completer &completer) {
	if(_running) throw std::runtime_error("Lua interpreter is busy");
	
	LuaCallResult res;
	res=loadChunk(reader,strName);
	if(!res.success) return completer(res);
	
	_running=true;
	
	auto lock=_taskCV.getLock();
	_task=completer;
	if(!_thread.joinable()) _thread=std::thread(&LuaServer::threadProc,this);
	else _taskCV.notify();
}

bool LuaServer::busy() const {
	return _running;
}

void LuaServer::wait() {
// Wait for the current task to finish
	auto lock=_runningCV.getLock();
	while(_running) _runningCV.wait(lock);
}

void LuaServer::terminate() {
	if(_ownState&&_running) _terminationRequested=true;
}

// Member functions to work with the Lua stack

void LuaServer::pushValue(const LuaValue &val) {
	switch(val.type()) {
	case LuaValue::Nil:
		lua_pushnil(_lua);
		break;
	case LuaValue::Boolean:
		lua_pushboolean(_lua,val.toBoolean()?1:0);
		break;
	case LuaValue::Number:
		lua_pushnumber(_lua,val.toNumber());
		break;
	case LuaValue::Integer:
		lua_pushinteger(_lua,val.toInteger());
		break;
	case LuaValue::String:
		lua_pushlstring(_lua,val.toString().data(),val.toString().size());
		break;
	case LuaValue::Array:
// lua_createtable() is preferred to lua_newtable() when we know table size in advance
		lua_createtable(_lua,static_cast<int>(val.size()),0);
		for(std::size_t i=0;i<val.size();i++) {
			lua_pushinteger(_lua,static_cast<lua_Integer>(i+1)); // key
			pushValue(val.array()[i]); // value
			lua_settable(_lua,-3); // create table element
		}
		break;
	case LuaValue::Table:
		lua_newtable(_lua); // create a new table and push it to stack
		for(auto it=val.table().cbegin();it!=val.table().cend();it++) {
			pushValue(it->first); // push key
			pushValue(it->second); // push value
			lua_settable(_lua,-3); // create table element
		}
		// push metatable
		if(val.metatable().size()) {
			lua_newtable(_lua);
			for(auto it=val.metatable().cbegin();it!=val.metatable().cend();it++) {
				pushValue(it->first); // push key
				pushValue(it->second); // push value
				lua_settable(_lua,-3); // create table element
			}
			lua_setmetatable(_lua,-2);
		}
		break;
	case LuaValue::CFunction:
		{
			std::vector<LuaValue> upvalues;
			lua_CFunction func=val.toCFunction(upvalues);
			for(auto it=upvalues.cbegin();it!=upvalues.cend();it++) {
				pushValue(*it);
			}
			lua_pushcclosure(_lua,func,static_cast<int>(upvalues.size()));
			break;
		}
	case LuaValue::LightUserData:
		lua_pushlightuserdata(_lua,val.toLightUserData());
		break;
	case LuaValue::FileHandle:
		{
			auto f=static_cast<luaL_Stream*>(lua_newuserdata(_lua,sizeof(luaL_Stream)));
			luaL_setmetatable(_lua,LUA_FILEHANDLE);
			f->f=val.toFileHandle();
			f->closef=closeFileHandle;
			break;
		}
	case LuaValue::Invalid:
		lua_pushnil(_lua);
		break;
	}
}

LuaValue LuaServer::pullValue(int stackpos,bool tableAsArray) {
	const char *sz;
	std::size_t size;
	int tindex,mtindex;
	
	const void* luaptr;
	
	switch(lua_type(_lua,stackpos)) {
	case LUA_TNIL:
		return LuaValue();
	case LUA_TBOOLEAN:
		return LuaValue(lua_toboolean(_lua,stackpos)!=0);
	case LUA_TNUMBER:
		if(lua_isinteger(_lua,stackpos)) return LuaValue(lua_tointeger(_lua,stackpos));
		return LuaValue(lua_tonumber(_lua,stackpos));
	case LUA_TSTRING:
		sz=lua_tolstring(_lua,stackpos,&size);
		return LuaValue(sz,size);
	case LUA_TTABLE:
		if(tableAsArray) {
			LuaValue t;
			auto &arr=t.newarray();
			tindex=lua_absindex(_lua,stackpos);
			const int len=static_cast<int>(lua_rawlen(_lua,stackpos));
			arr.reserve(len);
			for(int i=1;i<=len;i++) {
				lua_pushinteger(_lua,static_cast<lua_Integer>(i));
				lua_rawget(_lua,tindex);
				arr.push_back(popValue(true));
			}
			return t;
		}
		else {
			LuaValue t;
			auto &table=t.newtable();
			auto &meta=t.metatable();

// Table stack index can be relative, let's make it absolute
			tindex=lua_absindex(_lua,stackpos);

// Check that we don't have infinite recursion
			luaptr=lua_topointer(_lua,-1);
			if(_traversedtables.find(luaptr)!=_traversedtables.end()) return LuaValue::invalid();

// Add pointer to the current table to the list
			_traversedtables.insert(luaptr);

			lua_pushnil(_lua);
			while(lua_next(_lua,tindex)) {
				table.emplace(pullValue(-2),pullValue(-1));
				lua_pop(_lua,1); // pop value; retain key for the next iteration
			}

// Pull metatable (if any)
			if(lua_getmetatable(_lua,tindex)) {
				mtindex=lua_absindex(_lua,-1);
				lua_pushnil(_lua);
				while(lua_next(_lua,mtindex)) {
					meta.emplace(pullValue(-2),pullValue(-1));
					lua_pop(_lua,1); // pop value; retain key for the next iteration
				}
				lua_pop(_lua,1); // pop metatable
			}

// Remove pointer to the current table from the list
			_traversedtables.erase(luaptr);
			return t;
		}
	case LUA_TFUNCTION:
		{
			if(!lua_iscfunction(_lua,stackpos)) return LuaValue::invalid(); // can't pull Lua functions yet

// Check that we don't have infinite recursion
			luaptr=lua_topointer(_lua,-1);
			if(_traversedtables.find(luaptr)!=_traversedtables.end()) return LuaValue::invalid();
			
// Add pointer to the current cfunction to the list
			_traversedtables.insert(luaptr);
			
// Get upvalues
			std::vector<LuaValue> upvalues;
			for(int i=1;;i++) {
				sz=lua_getupvalue(_lua,stackpos,i);
				if(!sz) break;
				upvalues.push_back(popValue());
			}
// Remove pointer to the current cfunction from the list
			_traversedtables.erase(luaptr);
			
			return LuaValue(lua_tocfunction(_lua,stackpos),upvalues);
		}
	case LUA_TLIGHTUSERDATA:
		return LuaValue(lua_touserdata(_lua,stackpos));
	case LUA_TUSERDATA:
		{
			auto f=static_cast<luaL_Stream*>(luaL_testudata(_lua,stackpos,LUA_FILEHANDLE));
			if(!f) return LuaValue::invalid();
			return LuaValue(f->f);
		}
	default: // LUA_TNONE and unsupported types
		return LuaValue::invalid();
	}
}

LuaValue LuaServer::popValue(bool tableAsArray) {
	LuaValue v=pullValue(-1,tableAsArray);
	if(lua_gettop(_lua)>0) lua_pop(_lua,1);
	return v;
}

LuaValue::Type LuaServer::valueType(int stackPos) {
	switch(lua_type(_lua,stackPos)) {
	case LUA_TNIL:
		return LuaValue::Nil;
	case LUA_TBOOLEAN:
		return LuaValue::Boolean;
	case LUA_TNUMBER:
		if(lua_isinteger(_lua,stackPos)) return LuaValue::Integer;
		return LuaValue::Number;
	case LUA_TSTRING:
		return LuaValue::String;
	case LUA_TTABLE:
		return LuaValue::Table;
	case LUA_TFUNCTION:
		if(!lua_iscfunction(_lua,stackPos)) return LuaValue::Invalid;
		return LuaValue::CFunction;
	case LUA_TLIGHTUSERDATA:
		return LuaValue::LightUserData;
	case LUA_TUSERDATA:
	{
		auto f=static_cast<luaL_Stream*>(luaL_testudata(_lua,stackPos,LUA_FILEHANDLE));
		if(!f) return LuaValue::Invalid;
		return LuaValue::FileHandle;
	}
	default: // LUA_TNONE and unsupported types
		return LuaValue::Invalid;
	}
}

LuaIterator LuaServer::getIterator(int stackPos,const LuaValue &firstKey) {
	return LuaIterator(*this,stackPos,firstKey);
}

bool LuaServer::isValidIndex(int stackpos) {
	if(lua_type(_lua,stackpos)==LUA_TNONE) return false;
	return true;
}

void LuaServer::clearstack() {
	if(lua_gettop(_lua)>0) lua_pop(_lua,lua_gettop(_lua));
}

// Members to set/get Lua global variables

void LuaServer::setGlobal(const std::string &name,const LuaValue &val) {
	pushValue(val);
	lua_setglobal(_lua,name.c_str());
}

LuaValue LuaServer::getGlobal(const std::string &name) {
	lua_getglobal(_lua,name.c_str());
	return popValue();
}

LuaIterator LuaServer::getGlobalIterator(const LuaValue &firstKey) {
	lua_pushglobaltable(_lua);
	LuaIterator res(*this,-1,firstKey);
	lua_pop(_lua,1);
	return res;
}

// Members used by callbacks to access arguments and upvalues

int LuaServer::argc() {
	return lua_gettop(_lua);
}

LuaValue::Type LuaServer::argt(int i) {
	return valueType(i+1);
}

LuaValue LuaServer::argv(int i,bool tableAsArray) {
	return pullValue(i+1,tableAsArray);
}

int LuaServer::nupv() {
	int i;
// first three upvalues are intended for globalDispatcher()
	for(i=4;;i++) if(!isValidIndex(lua_upvalueindex(i))) break;
	return i-4;
}

LuaValue LuaServer::upvalues(int i) {
	return pullValue(lua_upvalueindex(i+4));
}

// Members providing debug information

std::string LuaServer::currentChunkName() {
	lua_Debug ar;
	for(int i=0;;i++) {
		int r=lua_getstack(_lua,i,&ar);
		if(!r) return "";
		lua_getinfo(_lua,"S",&ar);
		if(strcmp(ar.what,"main")&&strcmp(ar.what,"Lua")) continue;
		return ar.source;
	}
}

bool LuaServer::checkRuntime() {
// Try to check whether LuaServer and Lua are using the same standard C runtime library
	int retVals,errCode;
	int oldErrno=errno;
	bool res=true;

// Note: luaL_fileresult returns errno as a third return value (see lauxlib.c)
	errno=ENOENT;
	retVals=luaL_fileresult(_lua,0,"");
	errCode=static_cast<int>(pullValue(-1).toInteger());
	lua_pop(_lua,retVals);
	if(errCode!=ENOENT) res=false;
	
	errno=EACCES;
	retVals=luaL_fileresult(_lua,0,"");
	errCode=static_cast<int>(pullValue(-1).toInteger());
	lua_pop(_lua,retVals);
	if(errCode!=EACCES) res=false;
	
	errno=oldErrno;

	return res;
}

// Members to control callback registry

// Note: registerCallback() must be thread-safe

LuaValue LuaServer::registerCallback(typeLuaCallback ptr,const std::vector<LuaValue> &upvalues) {
	lua_Integer uniqueId=makeUniqueId();
	
	Registry::lock_t reglock(_reg.m);
	
	LuaDispatchData &record=_reg.dispatchRecords[uniqueId];
	record.type=LuaDispatchData::function;
	record.pfunc=ptr;
	
	auto upv=auxvalues(uniqueId,false);
	upv.insert(upv.end(),upvalues.begin(),upvalues.end());
		
	return LuaValue(stackGuard,upv);
}

// Note: unregisterCallback() must be thread-safe

void LuaServer::unregisterCallback(const LuaValue &cb) {
	std::vector<LuaValue> upvalues;
	
	Registry::lock_t reglock(_reg.m);
	
// Argument must be of CFUNCTION type
	assert(cb.type()==LuaValue::CFunction);
	cb.toCFunction(upvalues);
	if(upvalues.size()<2) throw std::runtime_error("No upvalue with LuaDispatchData unique ID");

// First upvalue should contain pointer to this
	if(upvalues[0].type()!=LuaValue::LightUserData)
		throw std::runtime_error("Wrong upvalue type for unregisterCallback()");
	if(static_cast<LuaServer*>(upvalues[0].toLightUserData())!=this)
		throw std::runtime_error("unregisterCallback(): Lua server doesn't match");
	
// Second upvalue should contain LIGHTUSERDATA object which points to the LuaDispatchData structure
	if(upvalues[1].type()!=LuaValue::Integer)
		throw std::runtime_error("Wrong upvalue type for unregisterCallback()");
	_reg.dispatchRecords.erase(upvalues[1].toInteger());
}

// Note: registerObject() must be thread-safe

LuaValue LuaServer::registerObject(LuaCallbackObject &obj) {
	LuaValue t;
	std::function<int(LuaServer&)> invoker;
	std::string name;
	std::vector<LuaValue> upvalues;
	
	Registry::lock_t reglock(_reg.m);
	
	t.newtable();
	
	for(int i=0;;i++) {
		name.clear();
		upvalues.clear();
		invoker=obj.enumerateLuaMethods(i,name,upvalues);
		if(!invoker) break;
		if(name.empty()) break;
		
		lua_Integer uniqueId=makeUniqueId();
		
		_reg.registeredObjects.emplace(&obj,uniqueId);
		LuaDispatchData &record=_reg.dispatchRecords[uniqueId];
		record.type=LuaDispatchData::object;
		record.invoker=invoker;
		record.pobj=&obj;
		
/*
 * we ignore dispatch errors in the __gc metamethod because the object
 * can be destroyed manually prior to garbage collection
 * we also ignore dispatch errors in the __close metamethod because it
 * can be called multiple times
 */
		auto upv=auxvalues(uniqueId,(name=="__gc"||name=="__close")?true:false); 
		upv.insert(upv.end(),upvalues.begin(),upvalues.end());
		
		if(name.substr(0,2)!="__") t.table()[name]=LuaValue(stackGuard,upv); // plain method
		else t.metatable()[name]=LuaValue(stackGuard,upv); // metamethod
	}
	
	t.table().emplace("type",obj.objectType());
	
	return t;
}

// Note: unregisterObject() must be thread-safe

void LuaServer::unregisterObject(LuaCallbackObject &obj) {
	Registry::lock_t reglock(_reg.m);
	
	auto records=_reg.registeredObjects.equal_range(&obj);
	
	if(records.first==records.second) return; // already unregistered
	
	auto cb=obj._cb;

	for(;;) {
// If unregistration has been explicitly allowed from this thread, proceed with unregistration
		if(cb->enableUnsafeDestruction&&cb->unsafeDestructionThread==std::this_thread::get_id()) break;
// If no callback is being executed on this object, proceed with unregistration
		if(cb->callbackThreadStack.empty()) break;
// If a callback is being executed on the same thread, proceed with unregistration
		if(cb->callbackThreadStack.back()==std::this_thread::get_id()) break;
// Otherwise, wait for callback completion
		_reg.cv.wait(reglock);
	}
	
	if(_reg.registeredObjects.count(&obj)==0) return; // already unregistered
	
	for(auto it=records.first;it!=records.second;it++) {
		_reg.dispatchRecords.erase(it->second);
	}
	_reg.registeredObjects.erase(records.first,records.second);
}

// Note: addManagedObject() must be thread-safe

LuaValue LuaServer::addManagedObject(LuaCallbackObject *newObj) {
	Registry::lock_t reglock(_reg.m);
	
	_reg.managedObjects.insert(newObj);
	newObj->_cb->owner=this;
	
	LuaValue lo=registerObject(*newObj);
	
	assert(lo.type()==LuaValue::Table);
	
	if(lo.metatable().find("__gc")==lo.metatable().end()) {
// Define Lua "garbage collection" metamethod
		lua_Integer uniqueId=makeUniqueId();
		
		_reg.registeredObjects.emplace(newObj,uniqueId);
		LuaDispatchData &record=_reg.dispatchRecords[uniqueId];
		record.type=LuaDispatchData::object;
		record.invoker=std::bind(&LuaCallbackObject::gcDispose,newObj,std::placeholders::_1);
		record.pobj=newObj;
		
		auto upv=auxvalues(uniqueId,true);
		
		lo.metatable()["__gc"]=LuaValue(stackGuard,upv);
	}
	
	if(lo.metatable().find("__close")==lo.metatable().end()) {
// Define Lua 5.4 "to close" metamethod
		lua_Integer uniqueId=makeUniqueId();
		
		_reg.registeredObjects.emplace(newObj,uniqueId);
		LuaDispatchData &record=_reg.dispatchRecords[uniqueId];
		record.type=LuaDispatchData::object;
		record.invoker=std::bind(&LuaCallbackObject::gcDispose,newObj,std::placeholders::_1);
		record.pobj=newObj;
		
		auto upv=auxvalues(uniqueId,true);
		
		lo.metatable()["__close"]=LuaValue(stackGuard,upv);
	}
	
	return lo;
}

// Note: detachManagedObject() must be thread-safe

void LuaServer::detachManagedObject(LuaCallbackObject *pObj) {
	unregisterObject(*pObj);
	Registry::lock_t reglock(_reg.m);
	_reg.managedObjects.erase(pObj);
}

/***************************************
 * LuaServer protected members
 ***************************************/

LuaCallResult LuaServer::execute() {
	LuaCallResult res;
	
	_finalizers.push_back(std::vector<std::function<void()> >()); // push new finalizer vector
	
	int r=lua_pcall(_lua,0,LUA_MULTRET,0);
	if(r) {
		res.errorMessage="Lua runtime error: ";
		if(lua_type(_lua,-1)==LUA_TSTRING) res.errorMessage+=lua_tostring(_lua,-1);
		else res.errorMessage+="Undefined error";
		executeFinalizers();
		clearstack();
		return res;
	}
	
	res.success=true;
	
	assert(_traversedtables.empty());
	
	for(int i=1;i<=lua_gettop(_lua);i++) {
		res.results.push_back(pullValue(i));
	}
	
	assert(_traversedtables.empty());
	
	executeFinalizers();
	if(_autoClearStack) clearstack();
	
	return res;
}

/***************************************
 * LuaServer private members
 ***************************************/

LuaCallResult LuaServer::loadChunk(const std::string &strChunk,const std::string &strName) {
	LuaCallResult res;
	
	int r=luaL_loadbuffer(_lua,strChunk.c_str(),strChunk.size(),strName.c_str());
	if(r) {
		res.errorMessage="Lua parser error: ";
		if(lua_type(_lua,-1)==LUA_TSTRING) res.errorMessage+=lua_tostring(_lua,-1);
		else res.errorMessage+="Undefined error";
/*
 * Determine whether the error was caused by incomplete user input,
 * similarly to the incomplete() function from lua.c. The last condition
 * is for Lua 5.1, which is not supported by SDM anymore, but I don't want
 * to remove this condition yet.
 */
		if(r==LUA_ERRSYNTAX) {
			if(StringUtils::endsWith(res.errorMessage,"near <eof>")) res.incomplete=true;
			else if(StringUtils::endsWith(res.errorMessage,"near \'<eof>\'")) res.incomplete=true;
		}
		
		clearstack();
		return res;
	}
	
	res.success=true;
	return res;
}

LuaCallResult LuaServer::loadChunk(LuaStreamReader &reader,const std::string &strName) {
	LuaCallResult res;
	
	int r=lua_load(_lua,LuaStreamReader::readerFunc,&reader,strName.c_str(),nullptr);
	if(r) {
		res.errorMessage="Lua parser error: ";
		if(lua_type(_lua,-1)==LUA_TSTRING) res.errorMessage+=lua_tostring(_lua,-1);
		else res.errorMessage+="Undefined error";
		
// See the comment above
		if(r==LUA_ERRSYNTAX) {
			if(StringUtils::endsWith(res.errorMessage,"near <eof>")) res.incomplete=true;
			else if(StringUtils::endsWith(res.errorMessage,"near \'<eof>\'")) res.incomplete=true;
		}
		
		clearstack();
		return res;
	}
	
	res.success=true;
	return res;
}

void LuaServer::threadProc() {
	for(;;) {
// Wait for new task
		auto lock=_taskCV.getLock();
		while(!_task&&!_finishWorkerThread) _taskCV.wait(lock);
		if(_finishWorkerThread) break;
		
// Obtain completer object, release lock
		Completer completer;
		std::swap(completer,_task); // _task is now an empty object
		lock.unlock();
		
// Execute task
		auto res=execute();
		
// Clear _running flag, synchronization is needed for wait()
		auto rlock=_runningCV.getLock();
		_running=false;
		_runningCV.notify();
		
// Execute completer, ignore exceptions
// Note: interpreter is considered ready while completer is being executed
		try {
			completer(res);
		}
		catch(std::exception &) {}
	}
}

/*
 * Handling errors in C++ callbacks called from Lua code is tricky.
 * 
 * Executive summary: Lua is compiled as ANSI C, all callbacks must work
 * through the unified callback infrastructure implemented below
 * (registerCallback() and registerObject()), errors should be reported
 * by throwing a C++ exception (of std::exception type or derived from
 * it). User callback code absolutely MUST NOT call lua_error() or
 * related functions such as luaL_error(). In fact, it can be better
 * not to provide such a possibility in the first place, encapsulating
 * all Lua interface within the LuaServer class.
 * 
 * The rationale follows.
 * 
 * THE PROBLEM
 * 
 * When Lua is compiled as ANSI C, it uses setjmp/longjmp technique for
 * its error handling. Unfortunately this approach doesn't work well with
 * C++ code. The problem is twofold:
 * 
 * 1) If the callback execution results in an uncaught exception, it
 * propagates through the Lua code to the calling C++ code, i.e. the code
 * that called lua_call() or lua_pcall(). Lua doesn't have any means to
 * intercept the exception, so its functions are terminated abnormally
 * which can potentially lead to resource leaks. This issue is somewhat
 * less problematic since we can always wrap the callback in a try-catch
 * block to disallow exception propagation through Lua code.
 * 
 * 2) If the callback encounters an error, it should report it to the
 * calling Lua code. The only way of reporting is to call lua_error()
 * or some wrapper function like luaL_error(). These functions invoke
 * longjmp() and thus never return. According to the C++03 standard
 * (ISO/IEC 14882:2003), longjmp() behavior is undefined if it
 * terminates a function which has automatic objects which would be
 * destroyed if an exception were thrown (see subclause 18.7.4).
 * In other words, the standard doesn't guarantee that destructors for
 * local objects with automatic storage duration will be called
 * on longjmp() invocation. Some compilers may implement C++-aware
 * longjmp(), but this should not be relied upon. Bypassing destructors
 * for local objects in a C++ callback leads to resource leaks (such
 * leaks have been observed in practice). Actually, if we catch an
 * exception and use the message stored in it (std::exception::what())
 * to raise a Lua error, the exception object is precisely what gets
 * leaked (observed at least with GCC 4.8).
 * 
 * THE SOLUTION
 * 
 * One possible solution is to compile Lua as C++ code. In this case Lua
 * will use C++ exceptions (instead of setjmp/longjmp) for its error
 * handling, which completely avoids the aforementioned longjmp() problem.
 * When using this approach we still don't want to propagate exceptions
 * through Lua code since the exception will be caught by the Lua
 * interpreter and all information about it will be lost. So, in this
 * case we should wrap callbacks in a try-catch block and report any
 * errors through lua_error() or its analogs. Calling lua_error() is
 * safe when Lua is compiled as C++.
 * 
 * The main drawback of such a solution is reduced interoperability.
 * Since Lua is usually deployed as a plain C library, using Lua
 * installed on the system is most likely out of question. The
 * application has to rely on its own copy of Lua. This also rules
 * out using system-level package managers to install Lua addons.
 * 
 * This module implements another solution which allows the C++ code
 * to interoperate with Lua compiled in plain C mode (though it doesn't
 * preclude compiling Lua as C++ if desired).
 * 
 * The main idea is to have exactly one unified callback function which
 * then dispatches the call to a respective user function. The needed
 * information is provided in a so-called "C closure" which allows the
 * user to associate some arbitrary values with the callback function.
 * These values are then accessible from within the callback and are
 * used to obtain the target function pointer.
 * 
 * The unified callback uses two functions. The inner function,
 * globalDispatcher(), performs the actual dispatching. It is wrapped
 * in a try-catch block to catch any unhandled exception. This function
 * never calls lua_error(), instead in the case of error it pushes an
 * error message on the Lua stack and returns -1 (which normally can't
 * be returned by a callback).
 * 
 * The outer function, stackGuard(), just calls globalDispatcher() and
 * analyzes its return value. Any non-negative value is passed to Lua,
 * negative value triggers lua_error() invocation. stackGuard() is
 * implemented in a separate translation unit to prevent the compiler
 * from optimizing it away. Calling lua_error() from stackGuard() is
 * safe since it doesn't have any local objects besides POD ("plain
 * old data") variables which don't require destruction.
 * 
 * In order for this to be effective, the user code must not call
 * lua_error() or related functions, using C++ exceptions instead
 * to raise an error.
 */

// Note: LuaServer::globalDispatcher() is a static member of LuaServer

int LuaServer::globalDispatcher(lua_State *L) try {
	LuaServer *lua;
	lua_Integer uniqueId;
	int grace;
	int res;

/*
 * First three upvalues are intended for globalDispatcher() consumption:
 *
 * 1) LIGHTUSERDATA: a pointer to the Lua server that registered the callback.
 * 2) INTEGER: an unique ID used as a key to find LuaDispatchData structure
 * that contains all the required information to perform dispatch.
 * 3) BOOLEAN: "true" indicates that globalDispatcher() should fail
 * quietly if dispatch is impossible (used for garbage collection
 * metamethod).
 *
 * If there are any other upvalues after the first three, they are passed
 * to the the user function.
 */
	assert(lua_type(L,lua_upvalueindex(1))==LUA_TLIGHTUSERDATA);
	assert(lua_type(L,lua_upvalueindex(2))==LUA_TNUMBER);
	assert(lua_type(L,lua_upvalueindex(3))==LUA_TBOOLEAN);
	
	lua=static_cast<LuaServer*>(lua_touserdata(L,lua_upvalueindex(1)));
	uniqueId=lua_tointeger(L,lua_upvalueindex(2));
	grace=lua_toboolean(L,lua_upvalueindex(3));
	
	assert(lua);
	
// Check for termination request
	if(lua->_terminationRequested) {
		lua->_terminationRequested=false;
		throw std::runtime_error("Termination has been requested by the user");
	}
	
	Registry::lock_t reglock(lua->_reg.m);
	auto it=lua->_reg.dispatchRecords.find(uniqueId);
	if(it==lua->_reg.dispatchRecords.end()) {
		if(!grace) throw std::runtime_error("No such object/function or it has been deleted");
		return 0;
	}
	auto d=it->second;
	
// Plain function or object?
	if(d.type==LuaDispatchData::function) {
		reglock.unlock();
		res=(d.pfunc)(*lua);
	}
	else {
		auto cb=d.pobj->_cb;
		cb->callbackThreadStack.push_back(std::this_thread::get_id());
		
		std::unique_lock<LuaCallbackObject::callback_mutex_t> cblock;
		auto cbiterator=lua->_lockedMutexes.end();
// Lock the object's callback mutex (if it is set and not already locked)
		if(cb->callbackMutex) {
			if(lua->_lockedMutexes.find(cb->callbackMutex)==lua->_lockedMutexes.end()) {
				cblock=std::unique_lock<LuaCallbackObject::callback_mutex_t>(*cb->callbackMutex);
			}
			cbiterator=lua->_lockedMutexes.insert(cb->callbackMutex);
		}
		
// Release the registry mutex before entering callback
		reglock.unlock();
// Execute callback
		try {
			res=d.invoker(*lua);
		}
		catch(...) {
			reglock.lock();
			cb->callbackThreadStack.pop_back();
			if(cbiterator!=lua->_lockedMutexes.end()) lua->_lockedMutexes.erase(cbiterator);
			lua->_reg.cv.notify_all();
			throw;
		}
		
		reglock.lock();
		cb->callbackThreadStack.pop_back();
		if(cbiterator!=lua->_lockedMutexes.end()) lua->_lockedMutexes.erase(cbiterator);
		lua->_reg.cv.notify_all();
	}
	
	if(res<0) {
		if(!grace) throw std::runtime_error("Callback returned an invalid value");
		return 0;
	}
	
	return res;
}
catch(std::exception &ex) {
// Construct an error message
	luaL_where(L,1); // obtain current position
	lua_pushstring(L,ex.what()); // push error message
	lua_concat(L,2); // concatenate them
	return -1;
}

// Note: globalHook() is a static member of LuaServer

int LuaServer::globalHook(lua_State *L,lua_Debug *ar) try {
// Get LuaServer pointer from the registry
	lua_pushstring(L,"SDMLuaServerPointer");
	lua_rawget(L,LUA_REGISTRYINDEX);
	auto lua=static_cast<LuaServer*>(lua_touserdata(L,-1));
	lua_pop(L,1);
	
	if(lua->_terminationRequested) {
		lua->_terminationRequested=false;
		throw std::runtime_error("Termination has been requested by the user");
	}
	return 0;
}
catch(std::exception &ex) {
// Construct an error message
	luaL_where(L,1); // obtain current position
	lua_pushstring(L,ex.what()); // push error message
	lua_concat(L,2); // concatenate them
	return -1;
}

// Note: closeFileHandle() is a static member of LuaServer

int LuaServer::closeFileHandle(lua_State *L) try {
	auto f=static_cast<luaL_Stream*>(luaL_testudata(L,1,LUA_FILEHANDLE));
	if(!f) throw std::runtime_error("Not a file handle");
	auto r=fclose(f->f);
	if(r) throw std::runtime_error("fclose() failed");
	lua_pushboolean(L,1);
	return 1;
}
catch(std::exception &ex) {
// Something is wrong
	lua_pushnil(L);
	lua_pushstring(L,ex.what());
	return 2;
}

std::vector<LuaValue> LuaServer::auxvalues(lua_Integer id,bool grace) {
	std::vector<LuaValue> upvalues;
	
	// 1st upvalue: pointer to the LuaServer
	upvalues.emplace_back(this);
	// 2nd upvalue: Unique Id
	upvalues.emplace_back(id);
	// 3rd upvalue: fail gracefully?
	upvalues.emplace_back(grace);
	
	return upvalues;
}

// Note: makeUniqueId() must be thread-safe

lua_Integer LuaServer::makeUniqueId() {
	Registry::lock_t reglock(_reg.m);
	while(_reg.dispatchRecords.find(++_reg.lastUniqueId)!=_reg.dispatchRecords.end());
	return _reg.lastUniqueId;
}

void LuaServer::executeFinalizers() {
	while(!_finalizers.back().empty()) {
		_finalizers.back().back()();
		_finalizers.back().pop_back();
	}
	_finalizers.pop_back();
}

/***************************************
 * LuaServer::Registry members
 ***************************************/

/*
 * Note: we want to clear registry before Lua state is closed, therefore,
 * Registry::clear() must be called from LuaServer::~LuaServer().
 * 
 * In Registry::~Registry we check that the registry was indeed cleared.
 */

LuaServer::Registry::~Registry() {
	lock_t lock(m);
	assert(dispatchRecords.empty());
	assert(registeredObjects.empty());
	assert(managedObjects.empty());
}

void LuaServer::Registry::clear() {
	lock_t lock(m);
	dispatchRecords.clear();
	registeredObjects.clear();
	for(auto it=managedObjects.cbegin();it!=managedObjects.cend();it++) {
//prevent detachManagedObject() from being invoked by the LuaCallbackObject's destructor
		(*it)->_cb->owner=nullptr;
		delete *it;
	}
	managedObjects.clear();
}
