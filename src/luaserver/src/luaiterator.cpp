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
 * This module provides an implementation of the LuaIterator class
 * members.
 */

#include "luaiterator.h"
#include "luaserver.h"

/*
 * UniqueRef members
 */

const int LuaIterator::UniqueRef::KeyRef=1;
const int LuaIterator::UniqueRef::TableRef=2;

LuaIterator::UniqueRef::UniqueRef(lua_State *L): _state(L) {
// Stack: [-1]: key to reference, [-2]: table
// Creates a reference. Pops arguments from the stack
	lua_createtable(_state,2,0); // push reference table
	lua_insert(_state,-3); // move it below the arguments
	lua_rawseti(_state,-3,KeyRef);
	lua_rawseti(_state,-2,TableRef);
	lua_rawsetp(_state,LUA_REGISTRYINDEX,this);
}

LuaIterator::UniqueRef::~UniqueRef() {
// Remove reference from the registry
	lua_pushnil(_state);
	lua_rawsetp(_state,LUA_REGISTRYINDEX,this);
}

void LuaIterator::UniqueRef::pushTableAndKey() {
	lua_rawgetp(_state,LUA_REGISTRYINDEX,this);
	lua_rawgeti(_state,-1,TableRef); // [-1]: table, [-2]: reftable
	lua_rawgeti(_state,-2,KeyRef); // [-1]: key, [-2]: table, [-3]: reftable
	lua_remove(_state,-3); // remove reftable
}

/*
 * LuaIterator members
 */

LuaIterator::LuaIterator(): _lua(nullptr),_state(nullptr) {}

LuaIterator::LuaIterator(LuaServer &l,int stackPos,const LuaValue &firstKey):
	_lua(&l),
	_state(l.state()),
	_evaluated(false)
{
	if(lua_type(_state,stackPos)!=LUA_TTABLE) return;
	lua_pushvalue(_state,stackPos); // push table
	if(firstKey.type()==LuaValue::Nil) { // get first key
		lua_pushnil(_state);
		int r=lua_next(_state,-2);
		if(r) {
			lua_pop(_state,1); // we don't need value
			_ref=std::make_shared<UniqueRef>(_state);
		}
		else lua_pop(_state,1); // pop table
	}
	else { // get given key
		_lua->pushValue(firstKey);
		_ref=std::make_shared<UniqueRef>(_state);
	}
}

LuaIterator &LuaIterator::operator++() {
	if(!_ref) return *this;
	_ref->pushTableAndKey();
	int r=lua_next(_state,-2);
	if(r) {
		lua_pop(_state,1); // don't need value
		_ref=std::make_shared<UniqueRef>(_state);
	}
	else {
		lua_pop(_state,1); // remove table
		_ref.reset(); // end
	}
	_evaluated=false;
	return *this;
}

LuaIterator LuaIterator::operator++(int) const {
	LuaIterator res(*this);
	return ++res;
}

LuaIterator::operator bool() const {
	return _ref.operator bool();
}

bool LuaIterator::operator==(const LuaIterator &other) {
	if(!_ref&&!other._ref) return true;
	if(static_cast<bool>(_ref)!=static_cast<bool>(other._ref)) return false;
	_ref->pushTableAndKey();
	other._ref->pushTableAndKey(); // [-1]: other key, [-2]: other table, [-3]: our key, [-4]: our table
	bool equal=true;
	if(lua_rawequal(_state,-2,-4)==0) equal=false;
	else if(lua_rawequal(_state,-1,-3)==0) equal=false;
	lua_pop(_state,4);
	return equal;
}

bool LuaIterator::operator!=(const LuaIterator &other) {
	return !operator==(other);
}

LuaIterator::reference LuaIterator::operator*() {
	if(!_evaluated) {
		if(!_ref) _value=std::make_pair(LuaValue::invalid(),LuaValue::invalid());
		else {
			_ref->pushTableAndKey();
			_value.first=_lua->pullValue(-1);
			lua_rawget(_state,-2);
			_value.second=_lua->pullValue(-1);
			lua_pop(_state,2);
		}
		_evaluated=true;
	}
	return _value;
}

LuaIterator::pointer LuaIterator::operator->() {
	operator*();
	return &_value;
}

LuaValue::Type LuaIterator::keyType() {
	if(!_ref) return LuaValue::Invalid;
	_ref->pushTableAndKey();
	auto type=_lua->valueType(-1);
	lua_pop(_state,2);
	return type;
}

LuaValue::Type LuaIterator::valueType() {
	if(!_ref) return LuaValue::Invalid;
	_ref->pushTableAndKey();
	lua_rawget(_state,-2);
	auto type=_lua->valueType(-1);
	lua_pop(_state,2);
	return type;
}

LuaValue LuaIterator::key() {
	if(!_ref) return LuaValue::invalid();
	_ref->pushTableAndKey();
	auto res=_lua->pullValue(-1);
	lua_pop(_state,2);
	return res;
}

LuaIterator LuaIterator::keyIterator(const LuaValue &firstKey) {
	if(!_ref) return LuaIterator();
	_ref->pushTableAndKey();
	LuaIterator res(*_lua,-1,firstKey);
	lua_pop(_state,2);
	return res;
}

LuaIterator LuaIterator::valueIterator(const LuaValue &firstKey) {
	if(!_ref) return LuaIterator();
	_ref->pushTableAndKey();
	lua_rawget(_state,-2);
	LuaIterator res(*_lua,-1,firstKey);
	lua_pop(_state,2);
	return res;
}

void LuaIterator::setValue(const LuaValue &val) {
	if(!_ref) return;
	_ref->pushTableAndKey();
	_lua->pushValue(val);
	lua_rawset(_state,-3);
	lua_pop(_state,1);
	_evaluated=false;
}
