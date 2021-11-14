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
 * This module implements Lua bindings for the DFIC library.
 */

#include "luadfic.h"
#include "luaserver.h"
#include "stringutils.h"

#include <cassert>
#include <cstring>
#include <utility>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <ctime>
#include <type_traits>

using namespace std::placeholders;

/*
 * Main Lua module interface
 */

static LuaServer lua(nullptr);

EXPORT int luaopen_luadfic(lua_State *L) {
// Check that there is only one Lua interpreter instance
	luaL_checkversion(L);

// Attach our LuaServer to the exisiting Lua state
	lua.attach(L);

// Create global managed object of UartLib type
	auto handle=lua.addManagedObject(new LuaDficLib);
	handle.table().emplace("nullptr",static_cast<void*>(nullptr));
	lua.pushValue(handle);
	
	return 1;
}

/*
 * LuaDficLib members
 */

std::function<int(LuaServer&)> LuaDficLib::enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
	switch(i) {
	case 0:
		strName="import";
		return std::bind(&LuaDficLib::LuaMethod_import,this,_1);
	case 1:
		strName="buffer";
		return std::bind(&LuaDficLib::LuaMethod_buffer,this,_1);
	case 2:
		strName="info";
		return std::bind(&LuaDficLib::LuaMethod_info,this,_1);
	case 3:
		strName="typesize";
		return std::bind(&LuaDficLib::LuaMethod_typesize,this,_1);
	case 4:
		strName="int2ptr";
		return std::bind(&LuaDficLib::LuaMethod_int2ptr,this,_1);
	case 5:
		strName="ptr2int";
		return std::bind(&LuaDficLib::LuaMethod_ptr2int,this,_1);
	default:
		return std::function<int(LuaServer&)>();
	}
}

int LuaDficLib::LuaMethod_import(LuaServer &lua) {
	if(lua.argc()!=2&&lua.argc()!=3) throw std::runtime_error("import() method takes 2-3 arguments");
	std::string module,symbol,prototype;
	
	if(lua.argt(0)!=LuaValue::Nil) module=lua.argv(0).toString();
	symbol=lua.argv(1).toString();
	if(lua.argc()==3&&lua.argt(2)!=LuaValue::Nil) prototype=lua.argv(2).toString();

	lua.pushValue(lua.addManagedObject(new LuaDfic(module,symbol,prototype)));
	return 1;
}

int LuaDficLib::LuaMethod_buffer(LuaServer &lua) {
	if(lua.argc()!=1&&lua.argc()!=2) throw std::runtime_error("buffer() method takes 1-2 arguments");
	
	LuaBuffer *buf;
	
	if(lua.argc()==1) buf=new LuaBuffer(static_cast<std::size_t>(lua.argv(0).toInteger()));
	else buf=new LuaBuffer(lua.argv(0).toLightUserData(),static_cast<std::size_t>(lua.argv(1).toInteger()));
	
	lua.pushValue(lua.addManagedObject(buf));
	
	return 1;
}

int LuaDficLib::LuaMethod_info(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("info() method takes 1 argument");
	
	auto const &str=lua.argv(0).toString();
	
	if(str=="os") {
#ifdef _WIN32
		lua.pushValue("windows");
#else
		lua.pushValue("unix");
#endif
	}
	else if(str=="cpu") {
#if defined(DFIC_CPU_X86)
		lua.pushValue("x86");
#elif defined(DFIC_CPU_X64)
		lua.pushValue("x64");
#else
		lua.pushValue("unsupported");
#endif
	}
	else if(str=="extension") {
#ifdef _WIN32
		lua.pushValue("dll");
#else
		lua.pushValue("so");
#endif
	}
	else throw std::runtime_error("Unknown info() key");
	
	return 1;
}

int LuaDficLib::LuaMethod_typesize(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("typesize() method takes 1 argument");
	CompoundType t(lua.argv(0).toString());
	lua.pushValue(static_cast<lua_Integer>(t.size()));
	return 1;
}

int LuaDficLib::LuaMethod_int2ptr(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("int2ptr() method takes 1 argument");
	auto p=reinterpret_cast<void*>(lua.argv(0).toInteger());
	lua.pushValue(p);
	return 1;
}

int LuaDficLib::LuaMethod_ptr2int(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("ptr2int() method takes 1 argument");
	auto ptr=lua.argv(0).toLightUserData();
	auto i=reinterpret_cast<lua_Integer>(ptr);
	auto ptr2=reinterpret_cast<void*>(i);
	if(ptr!=ptr2) throw std::runtime_error("Pointer value can't be represented as integer");
	lua.pushValue(i);
	return 1;
}

/*
 * LuaDfic public members
 */

LuaDfic::LuaDfic(const std::string &moduleName,const std::string &symbolName,const std::string &prototype):
	_module(moduleName)
{
// Decode prototype string
	if(!prototype.empty()) {
		auto const &split=StringUtils::splitString(prototype,'@');
		
		std::string returnTypeString;
		std::string callingConventionString;
		std::string parameterString;
		
		switch(split.size()) {
		case 1: // only parameter list
			parameterString=StringUtils::cleanupString(split[0]);
			break;
		case 2: // return value + parameter list
			returnTypeString=StringUtils::cleanupString(split[0]);
			parameterString=StringUtils::cleanupString(split[1]);
			break;
		case 3: // return value + calling convention + parameter list
			returnTypeString=StringUtils::cleanupString(split[0]);
			callingConventionString=StringUtils::cleanupString(split[1]);
			parameterString=StringUtils::cleanupString(split[2]);
			break;
		default:
			throw std::runtime_error("Malformed prototype string");
		}
		
// Return type
		if(!returnTypeString.empty()) _returnType=CompoundType(returnTypeString);
		
// Calling convention
		if(!callingConventionString.empty()) {
			if(callingConventionString=="cdecl") _interface.setCallingConvention(Dfic::CDecl);
			else if(callingConventionString=="pascal") _interface.setCallingConvention(Dfic::Pascal);
			else if(callingConventionString=="stdcall") _interface.setCallingConvention(Dfic::StdCall);
			else if(callingConventionString=="fastcall") _interface.setCallingConvention(Dfic::FastCall);
			else if(callingConventionString=="thiscall") _interface.setCallingConvention(Dfic::ThisCall);
			else if(callingConventionString=="x64") _interface.setCallingConvention(Dfic::X64Native);
			else if(callingConventionString=="winapi") {
#ifdef DFIC_CPU_X86
				_interface.setCallingConvention(Dfic::StdCall);
#else
				_interface.setCallingConvention(Dfic::X64Native);
#endif
			}
			else throw std::runtime_error("Unknown calling convention: \""+callingConventionString+"\"");
		}
	
// Parameter list
		if(!parameterString.empty()) {
			auto const &parNames=StringUtils::splitString(parameterString,',');
			if(parNames.size()!=1||parNames[0]!="void") {
				for(auto const &name: parNames) {
					CompoundType t(name);
					if(!t.pointer()&&t.base()==Void) throw std::runtime_error("Parameter type can't be void");
					_params.push_back(t);
				}
			}
		}
	}
	
// Load function
	Dfic::GenericFuncPtr func=nullptr;
	std::ostringstream errMsg;
	
	try {
		func=_module.getAddr(symbolName);
	}
	catch(std::exception &ex) {
// Error loading functions, try different decoration schemes
		errMsg<<ex.what();
		auto const &decorations=listDecoratedNames(symbolName);
		for(auto const &dname: decorations) {
			try {
				func=_module.getAddr(dname);
				break;
			}
			catch(std::exception &ex) {
				errMsg<<std::endl<<ex.what();
			}
		}
		if(!func) throw std::runtime_error(errMsg.str());
	}
	
	_interface.setFunctionPointer(func);
}

/*
 * LuaDfic protected members
 */

std::function<int(LuaServer&)> LuaDfic::enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
	switch(i) {
	case 0:
		strName="close";
		return std::bind(&LuaDfic::LuaMethod_close,this,_1);
	case 1:
		strName="invoke";
		return std::bind(&LuaDfic::LuaMethod_invoke,this,_1);
	case 2:
		strName="__call";
		return std::bind(&LuaDfic::LuaMethod_meta_call,this,_1);
	default:
		return std::function<int(LuaServer&)>();
	}
}

int LuaDfic::LuaMethod_close(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("close() method doesn't take arguments");
	delete this;
	return 0;
}

int LuaDfic::LuaMethod_invoke(LuaServer &lua) {
	return invoke(lua,false);
}

int LuaDfic::LuaMethod_meta_call(LuaServer &lua) {
	return invoke(lua,true);
}

/*
 * LuaDfic private members
 */

int LuaDfic::invoke(LuaServer &lua,bool meta) {
	int retVals=0;
	
	if(lua.argc()!=static_cast<int>(_params.size())+(meta?1:0))
		throw std::runtime_error("Wrong argument count ("+std::to_string(_params.size())+" expected)");
	
// Push arguments
	for(std::size_t i=0;i<_params.size();i++) {
		auto const &val=lua.argv(static_cast<int>(i)+(meta?1:0),true); // table as array
		auto const &str=val.toString();

// Fundamental type
		if(!_params[i].type.pointer()) {
			switch(_params[i].type.base()) {
			case VoidPtr:
				_interface.setArgument(i,val.toLightUserData());
				break;
			case Bool:
				_interface.setArgument(i,val.toBoolean());
				break;
			case Char:
				if(str.size()!=1) throw std::runtime_error("Bad argument #"+std::to_string(i));
				_interface.setArgument(i,str[0]);
				break;
			case WChar:
				_interface.setArgument(i,static_cast<wchar_t>(val.toInteger()));
				break;
			case Short:
				_interface.setArgument(i,static_cast<short>(val.toInteger()));
				break;
			case UShort:
				_interface.setArgument(i,static_cast<unsigned short>(val.toInteger()));
				break;
			case Int:
				_interface.setArgument(i,static_cast<int>(val.toInteger()));
				break;
			case UInt:
				_interface.setArgument(i,static_cast<unsigned int>(val.toInteger()));
				break;
			case Long:
				_interface.setArgument(i,static_cast<long>(val.toInteger()));
				break;
			case ULong:
				_interface.setArgument(i,static_cast<unsigned long>(val.toInteger()));
				break;
			case LongLong:
				_interface.setArgument(i,static_cast<long long>(val.toInteger()));
				break;
			case ULongLong:
				_interface.setArgument(i,static_cast<unsigned long long>(val.toInteger()));
				break;
			case Size:
				_interface.setArgument(i,static_cast<std::size_t>(val.toInteger()));
				break;
			case IntPtr:
				_interface.setArgument(i,static_cast<std::intptr_t>(val.toInteger()));
				break;
			case UIntPtr:
				_interface.setArgument(i,static_cast<std::uintptr_t>(val.toInteger()));
				break;
			case PtrDiff:
				_interface.setArgument(i,static_cast<std::ptrdiff_t>(val.toInteger()));
				break;
			case Clock:
				if(std::is_integral<std::clock_t>::value)
					_interface.setArgument(i,static_cast<std::clock_t>(val.toInteger()));
				else
					_interface.setArgument(i,static_cast<std::clock_t>(val.toNumber()));
				break;
			case Time:
				if(std::is_integral<std::time_t>::value)
					_interface.setArgument(i,static_cast<std::time_t>(val.toInteger()));
				else
					_interface.setArgument(i,static_cast<std::time_t>(val.toNumber()));
				break;
			case Int8:
				_interface.setArgument(i,static_cast<std::int8_t>(val.toInteger()));
				break;
			case UInt8:
				_interface.setArgument(i,static_cast<std::uint8_t>(val.toInteger()));
				break;
			case Int16:
				_interface.setArgument(i,static_cast<std::int16_t>(val.toInteger()));
				break;
			case UInt16:
				_interface.setArgument(i,static_cast<std::uint16_t>(val.toInteger()));
				break;
			case Int32:
				_interface.setArgument(i,static_cast<std::int32_t>(val.toInteger()));
				break;
			case UInt32:
				_interface.setArgument(i,static_cast<std::uint32_t>(val.toInteger()));
				break;
			case Int64:
				_interface.setArgument(i,static_cast<std::int64_t>(val.toInteger()));
				break;
			case UInt64:
				_interface.setArgument(i,static_cast<std::uint64_t>(val.toInteger()));
				break;
			case Float:
				_interface.setArgument(i,static_cast<float>(val.toNumber()));
				break;
			case Double:
				_interface.setArgument(i,static_cast<double>(val.toNumber()));
				break;
			default:
				assert(false);
			}
		}
// Pointer type
		else {
			if(val.type()==LuaValue::Nil||
				(val.type()==LuaValue::LightUserData&&
					val.toLightUserData()==nullptr))
			{
				_params[i].buf.reset();
				_interface.setArgument(i,static_cast<void*>(nullptr));
			}
			else {
				switch(_params[i].type.base()) {
				case VoidPtr:
					_params[i].buf.reset(new ArrayBuffer<void*>(val));
					break;
				case Bool:
					_params[i].buf.reset(new ArrayBuffer<bool>(val));
					break;
				case Char:
					_params[i].buf.reset(new ArrayBuffer<char>(val));
					break;
				case WChar:
					_params[i].buf.reset(new ArrayBuffer<wchar_t>(val));
					break;
				case Short:
					_params[i].buf.reset(new ArrayBuffer<short>(val));
					break;
				case UShort:
					_params[i].buf.reset(new ArrayBuffer<unsigned short>(val));
					break;
				case Int:
					_params[i].buf.reset(new ArrayBuffer<int>(val));
					break;
				case UInt:
					_params[i].buf.reset(new ArrayBuffer<unsigned int>(val));
					break;
				case Long:
					_params[i].buf.reset(new ArrayBuffer<long>(val));
					break;
				case ULong:
					_params[i].buf.reset(new ArrayBuffer<unsigned long>(val));
					break;
				case LongLong:
					_params[i].buf.reset(new ArrayBuffer<long long>(val));
					break;
				case ULongLong:
					_params[i].buf.reset(new ArrayBuffer<unsigned long long>(val));
					break;
				case Size:
					_params[i].buf.reset(new ArrayBuffer<std::size_t>(val));
					break;
				case IntPtr:
					_params[i].buf.reset(new ArrayBuffer<std::intptr_t>(val));
					break;
				case UIntPtr:
					_params[i].buf.reset(new ArrayBuffer<std::uintptr_t>(val));
					break;
				case PtrDiff:
					_params[i].buf.reset(new ArrayBuffer<std::ptrdiff_t>(val));
					break;
				case Clock:
					_params[i].buf.reset(new ArrayBuffer<std::clock_t>(val));
					break;
				case Time:
					_params[i].buf.reset(new ArrayBuffer<std::time_t>(val));
					break;
				case Int8:
					_params[i].buf.reset(new ArrayBuffer<std::int8_t>(val));
					break;
				case UInt8:
					_params[i].buf.reset(new ArrayBuffer<std::uint8_t>(val));
					break;
				case Int16:
					_params[i].buf.reset(new ArrayBuffer<std::int16_t>(val));
					break;
				case UInt16:
					_params[i].buf.reset(new ArrayBuffer<std::uint16_t>(val));
					break;
				case Int32:
					_params[i].buf.reset(new ArrayBuffer<std::int32_t>(val));
					break;
				case UInt32:
					_params[i].buf.reset(new ArrayBuffer<std::uint32_t>(val));
					break;
				case Int64:
					_params[i].buf.reset(new ArrayBuffer<std::int64_t>(val));
					break;
				case UInt64:
					_params[i].buf.reset(new ArrayBuffer<std::uint64_t>(val));
					break;
				case Float:
					_params[i].buf.reset(new ArrayBuffer<float>(val));
					break;
				case Double:
					_params[i].buf.reset(new ArrayBuffer<double>(val));
					break;
				default:
					assert(false);
				}
				_interface.setArgument(i,_params[i].buf->data());
			}
		}
	}
// Invoke foreign function
	_interface.invoke();

// Obtain return value
	if(_returnType.pointer()||_returnType.base()!=Void) {
		if(!_returnType.pointer()) {
			switch(_returnType.base()) {
			case VoidPtr:
				lua.pushValue(_interface.returnValue<void*>());
				break;
			case Bool:
				lua.pushValue(_interface.returnValue<bool>());
				break;
			case Char:
				lua.pushValue(std::string(_interface.returnValue<char>(),1));
				break;
			case WChar:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<wchar_t>()));
				break;
			case Short:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<short>()));
				break;
			case UShort:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<unsigned short>()));
				break;
			case Int:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<int>()));
				break;
			case UInt:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<unsigned int>()));
				break;
			case Long:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<long>()));
				break;
			case ULong:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<unsigned long>()));
				break;
			case LongLong:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<long long>()));
				break;
			case ULongLong:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<unsigned long long>()));
				break;
			case Size:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<std::size_t>()));
				break;
			case IntPtr:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<std::intptr_t>()));
				break;
			case UIntPtr:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<std::uintptr_t>()));
				break;
			case PtrDiff:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<std::ptrdiff_t>()));
				break;
			case Clock:
				if(std::is_integral<std::clock_t>::value)
					lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<std::clock_t>()));
				else
					lua.pushValue(static_cast<lua_Number>(_interface.returnValue<std::clock_t>()));
				break;
			case Time:
				if(std::is_integral<std::time_t>::value)
					lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<std::time_t>()));
				else
					lua.pushValue(static_cast<lua_Number>(_interface.returnValue<std::time_t>()));
				break;
			case Int8:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<std::int8_t>()));
				break;
			case UInt8:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<std::uint8_t>()));
				break;
			case Int16:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<std::int16_t>()));
				break;
			case UInt16:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<std::uint16_t>()));
				break;
			case Int32:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<std::int32_t>()));
				break;
			case UInt32:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<std::uint32_t>()));
				break;
			case Int64:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<std::int64_t>()));
				break;
			case UInt64:
				lua.pushValue(static_cast<lua_Integer>(_interface.returnValue<std::uint64_t>()));
				break;
			case Float:
				lua.pushValue(_interface.returnValue<float>());
				break;
			case Double:
				lua.pushValue(_interface.returnValue<double>());
				break;
			default:
				assert(false);
			}
		}
		else { // pointer
			if(_interface.returnValue<const void*>()==NULL) lua.pushValue(LuaValue());
			else {
				switch(_returnType.base()) {
				case VoidPtr:
					lua.pushValue(*_interface.returnValue<void**>());
					break;
				case Bool:
					lua.pushValue(*_interface.returnValue<bool*>());
					break;
				case Char:
					lua.pushValue(_interface.returnValue<const char*>());
					break;
				case WChar:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<wchar_t*>()));
					break;
				case Short:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<short*>()));
					break;
				case UShort:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<unsigned short*>()));
					break;
				case Int:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<int*>()));
					break;
				case UInt:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<unsigned int*>()));
					break;
				case Long:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<long*>()));
					break;
				case ULong:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<unsigned long*>()));
					break;
				case LongLong:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<long long*>()));
					break;
				case ULongLong:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<unsigned long long*>()));
					break;
				case Size:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<std::size_t*>()));
					break;
				case IntPtr:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<std::intptr_t*>()));
					break;
				case UIntPtr:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<std::uintptr_t*>()));
					break;
				case PtrDiff:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<std::ptrdiff_t*>()));
					break;
				case Clock:
					if(std::is_integral<std::clock_t>::value)
						lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<std::clock_t*>()));
					else
						lua.pushValue(static_cast<lua_Number>(*_interface.returnValue<std::clock_t*>()));
					break;
				case Time:
					if(std::is_integral<std::time_t>::value)
						lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<std::time_t*>()));
					else
						lua.pushValue(static_cast<lua_Number>(*_interface.returnValue<std::time_t*>()));
					break;
				case Int8:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<std::int8_t*>()));
					break;
				case UInt8:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<std::uint8_t*>()));
					break;
				case Int16:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<std::int16_t*>()));
					break;
				case UInt16:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<std::uint16_t*>()));
					break;
				case Int32:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<std::int32_t*>()));
					break;
				case UInt32:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<std::uint32_t*>()));
					break;
				case Int64:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<std::int64_t*>()));
					break;
				case UInt64:
					lua.pushValue(static_cast<lua_Integer>(*_interface.returnValue<std::uint64_t*>()));
					break;
				case Float:
					lua.pushValue(*_interface.returnValue<float*>());
					break;
				case Double:
					lua.pushValue(*_interface.returnValue<double*>());
					break;
				default:
					assert(false);
				}
			}
		}
		
		retVals++;
	}

// Obtain output buffer contents
	for(std::size_t i=0;i<_params.size();i++) {
		if(_params[i].type.pointer()&&!_params[i].type.constant()) {
			if(!_params[i].buf) lua.pushValue(LuaValue());
			else lua.pushValue(_params[i].buf->toLua());
			retVals++;
		}
	}
	
	return retVals;
}

std::vector<std::string> LuaDfic::listDecoratedNames(const std::string &name) const {
#if defined(DFIC_CPU_X86) && defined(_WIN32)
	std::vector<std::string> res;
	
// List decoration schemes
	if(_interface.callingConvention()==Dfic::Pascal) { // pascal functions can be upper-cased
		res.push_back(name);
		for(auto &ch: res.back()) ch=std::toupper(ch);
	}
	else if(_interface.callingConvention()==Dfic::FastCall) res.push_back('@'+name);
	else res.push_back('_'+name);
	
	if(_interface.callingConvention()==Dfic::StdCall||
		_interface.callingConvention()==Dfic::FastCall)
	{
// Count number of bytes pushed on the stack
		std::size_t stackBytes=0;
		for(auto const &p: _params) {
			auto s=p.type.size();
			if(s%4!=0) s+=(4-s%4);
			stackBytes+=s;
		}
		
		auto decoratedName=name+'@'+std::to_string(stackBytes);
		if(_interface.callingConvention()==Dfic::FastCall) res.push_back('@'+decoratedName);
		else {
			res.push_back(decoratedName);
			res.push_back('_'+decoratedName);
		}
	}
	
	return res;
#else
	return std::vector<std::string>();
#endif
}

/*
 * LuaBuffer members
 */

LuaBuffer::LuaBuffer(std::size_t s) {
	if(s==0) throw std::runtime_error("Buffer size can't be zero");
	_ptr=new char[s](); // default-initialize
	_size=s;
	_own=true;
}

LuaBuffer::LuaBuffer(void *p,std::size_t s) {
	if(s==0) throw std::runtime_error("Buffer size can't be zero");
	_ptr=static_cast<char*>(p);
	_size=s;
	_own=false;
}

LuaBuffer::~LuaBuffer() {
	if(_own) delete[] _ptr;
}

std::function<int(LuaServer&)> LuaBuffer::enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
	switch(i) {
	case 0:
		strName="close";
		return std::bind(&LuaBuffer::LuaMethod_close,this,_1);
	case 1:
		strName="ptr";
		return std::bind(&LuaBuffer::LuaMethod_ptr,this,_1);
	case 2:
		strName="write";
		return std::bind(&LuaBuffer::LuaMethod_write,this,_1);
	case 3:
		strName="read";
		return std::bind(&LuaBuffer::LuaMethod_read,this,_1);
	case 4:
		strName="resize";
		return std::bind(&LuaBuffer::LuaMethod_resize,this,_1);
	case 5:
		strName="__len";
		return std::bind(&LuaBuffer::LuaMethod_meta_len,this,_1);
	case 6:
		strName="__index";
		return std::bind(&LuaBuffer::LuaMethod_meta_index,this,_1);
	case 7:
		strName="__newindex";
		return std::bind(&LuaBuffer::LuaMethod_meta_newindex,this,_1);
	default:
		return std::function<int(LuaServer&)>();
	}
}

int LuaBuffer::LuaMethod_close(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("close() method doesn't take arguments");
	delete this;
	return 0;
}

int LuaBuffer::LuaMethod_ptr(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("ptr() method doesn't take arguments");
	lua.pushValue(static_cast<void*>(_ptr));
	return 1;
}

int LuaBuffer::LuaMethod_write(LuaServer &lua) {
	if(lua.argc()!=1&&lua.argc()!=2) throw std::runtime_error("write() method takes 1-2 arguments");
	
	auto const &str=lua.argv(0).toString();
	std::size_t offset=0;
	if(lua.argc()>1) offset=static_cast<std::size_t>(lua.argv(1).toInteger()-1);
	if(offset+str.size()>_size) throw std::runtime_error("Out of range");
	
	std::memcpy(_ptr+offset,str.data(),str.size());
	return 0;
}

int LuaBuffer::LuaMethod_read(LuaServer &lua) {
	if(lua.argc()!=0&&lua.argc()!=2) throw std::runtime_error("read() method takes 0 or 2 arguments");
	
	if(lua.argc()==0) { // read null-terminated string
		if(std::memchr(_ptr,0,_size)) lua.pushValue(_ptr);
		else lua.pushValue(std::string(_ptr,_size));
	}
	else { // read range of characters
		std::size_t offset=static_cast<std::size_t>(lua.argv(0).toInteger()-1);
		std::size_t count=static_cast<std::size_t>(lua.argv(1).toInteger());
		if(offset+count>_size) throw std::runtime_error("Out of range");
		lua.pushValue(std::string(_ptr+offset,count));
	}
	
	return 1;
}

int LuaBuffer::LuaMethod_resize(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("resize() method takes 1 argument");
	if(!_own) throw std::runtime_error("Buffer is not owned");
	
	auto newSize=static_cast<std::size_t>(lua.argv(0).toInteger());
	if(newSize==0) throw std::runtime_error("Buffer size can't be zero");
	
	if(newSize<=_size) {
		_size=newSize;
	}
	else {
		char *newPtr=new char[newSize]();
		std::memcpy(newPtr,_ptr,std::min(_size,newSize));
		delete[] _ptr;
		_ptr=newPtr;
		_size=newSize;
	}
	
	lua.pushValue(static_cast<void*>(_ptr));
	
	return 1;
}

int LuaBuffer::LuaMethod_meta_len(LuaServer &lua) {
	lua.pushValue(static_cast<lua_Integer>(_size));
	return 1;
}

int LuaBuffer::LuaMethod_meta_index(LuaServer &lua) {
	assert(lua.argc()==2);
	auto pos=static_cast<std::size_t>(lua.argv(1).toInteger()-1);
	if(pos>=_size) throw std::runtime_error("Out of range");
	lua.pushValue(std::string(1,_ptr[pos]));
	return 1;
}

int LuaBuffer::LuaMethod_meta_newindex(LuaServer &lua) {
	assert(lua.argc()==3);
	auto pos=static_cast<std::size_t>(lua.argv(1).toInteger()-1);
	if(pos>=_size) throw std::runtime_error("Out of range");
	auto str=lua.argv(2).toString();
	if(str.size()!=1) throw std::runtime_error("String of size 1 expected");
	_ptr[pos]=str[0];
	return 0;
}
