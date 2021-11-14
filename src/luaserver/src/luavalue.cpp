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
 * This module provides an implementation of the LuaValue class
 * which is used to represent Lua values in C++ code.
 */

#include "luavalue.h"

#include <stdexcept>
#include <limits>
#include <sstream>
#include <cstdint>
#include <iomanip>

/*
 * Public members
 */

LuaValue::LuaValue(const LuaValue &orig) {
	switch(orig.t) {
	case Nil:
		break;
	case Boolean:
	case Number:
	case Integer:
		v=orig.v;
		break;
	case String:
		v.pstr=new std::string(*orig.v.pstr);
		break;
	case Array:
		v.parray=new std::vector<LuaValue>(*orig.v.parray);
		break;
	case Table:
		v.ptable=new LuaTable(*orig.v.ptable);
		break;
	case CFunction:
		v.pclosure=new LuaCClosure(*orig.v.pclosure);
		break;
	case LightUserData:
	case FileHandle:
		v=orig.v;
		break;
	case Invalid:
		break;
	}
	t=orig.t;
}

LuaValue::LuaValue(const std::string &s): t(String) {
	v.pstr=new std::string(s);
}

LuaValue::LuaValue(std::string &&s): t(String) {
	v.pstr=new std::string(std::move(s));
}

LuaValue::LuaValue(const char *sz): t(String) {
	v.pstr=new std::string(sz);
}

LuaValue::LuaValue(const char *sz,std::size_t len): t(String) {
	v.pstr=new std::string(sz,len);
}

LuaValue::LuaValue(const lua_CFunction p): t(CFunction) {
	v.pclosure=new LuaCClosure;
	v.pclosure->pf=p;
}

LuaValue::LuaValue(const lua_CFunction p, const std::vector<LuaValue> &upvalues): t(CFunction) {
	v.pclosure=new LuaCClosure;
	v.pclosure->pf=p;
	v.pclosure->upvalues=upvalues;
}

LuaValue::LuaValue(void *p): t(LightUserData) {
	v.plud=p;
}

LuaValue::~LuaValue() {
	if(t==String) delete v.pstr;
	else if(t==Array) delete v.parray;
	else if(t==Table) delete v.ptable;
	else if(t==CFunction) delete v.pclosure;
}

LuaValue &LuaValue::operator=(const LuaValue &right) {
	LuaValue temp(right);
	swap(temp);
	return *this;
}

std::string LuaValue::typeName() const {
	return typeName(t);
}

LuaValue &LuaValue::clear() {
	LuaValue temp; // new Nil object
	swap(temp);
	return *this;
}

// for std::map, all non-equal values must be strictly ordered
int LuaValue::compare(const LuaValue &right) const {
	if(t>right.t) return 1;
	if(t<right.t) return -1;
	
	switch(t) {
	case Nil:
		return 0;
	case Boolean:
		if(v.b&&!right.v.b) return 1;
		if(!v.b&&right.v.b) return -1;
		return 0;
	case Number:
		if(v.d>right.v.d) return 1;
		if(v.d<right.v.d) return -1;
		return 0;
	case Integer:
		if(v.i>right.v.i) return 1;
		if(v.i<right.v.i) return -1;
		return 0;
	case String:
		return v.pstr->compare(*right.v.pstr);
	case Array:
		if(*v.parray>*right.v.parray) return 1;
		if(*v.parray<*right.v.parray) return -1;
		return 0;
	case Table:
		if(v.ptable->dict>right.v.ptable->dict) return 1;
		if(v.ptable->dict<right.v.ptable->dict) return -1;
		if(v.ptable->meta>right.v.ptable->meta) return 1;
		if(v.ptable->meta<right.v.ptable->meta) return -1;
		return 0;
	case CFunction:
		if(v.pclosure->pf>right.v.pclosure->pf) return 1;
		if(v.pclosure->pf<right.v.pclosure->pf) return -1;
		if(v.pclosure->upvalues>right.v.pclosure->upvalues) return 1;
		if(v.pclosure->upvalues<right.v.pclosure->upvalues) return -1;
		return 0;
	case LightUserData:
		if(v.plud>right.v.plud) return 1;
		if(v.plud<right.v.plud) return -1;
		return 0;
	case FileHandle:
		if(v.fh>right.v.fh) return 1;
		if(v.fh<right.v.fh) return -1;
		return 0;
	case Invalid:
		return 0;
	}
	
	return 0;
}

size_t LuaValue::size() const {
	switch(t) {
	case Nil:
		return 0;
	case Boolean:
	case Number:
	case Integer:
		return 1;
	case String:
		return v.pstr->size();
	case Array:
		return v.parray->size();
	case Table:
		return v.ptable->dict.size();
	case CFunction:
	case LightUserData:
	case FileHandle:
		return 1;
	case Invalid:
		return 0;
	}
	return 0;
}

bool LuaValue::toBoolean() const {
	switch(t) {
	case Nil:
		return false;
	case Boolean:
		return v.b;
	case Number:
		return (v.d!=0);
	case Integer:
		return (v.i!=0);
	case String:
		return (v.pstr->size()!=0);
	case Array:
		return (v.parray->size()!=0);
	case Table:
		return (v.ptable->dict.size()!=0);
	case CFunction:
		return (v.pclosure->pf!=NULL);
	case LightUserData:
		return (v.plud!=NULL);
	case FileHandle:
		return (v.fh!=NULL);
	case Invalid:
		return false;
	}
	return false;
}

lua_Number LuaValue::toNumber() const {
	switch(t) {
	case Nil:
		return 0;
	case Boolean:
		return v.b?1:0;
	case Number:
		return v.d;
	case Integer:
		return static_cast<lua_Number>(v.i);
	case String:
		try {
			return std::stod(*v.pstr);
		}
		catch(std::exception &) {
			return std::numeric_limits<lua_Number>::quiet_NaN();
		}
	case Array:
	case Table:
	case CFunction:
	case LightUserData:
	case FileHandle:
	case Invalid:
		return std::numeric_limits<lua_Number>::quiet_NaN();;
	}
	return std::numeric_limits<lua_Number>::quiet_NaN();
}

lua_Integer LuaValue::toInteger() const {
	switch(t) {
	case Nil:
		return 0;
	case Boolean:
		return v.b?1:0;
	case Number:
		return static_cast<lua_Integer>(v.d);
	case Integer:
		return v.i;
	case String:
		try {
			return static_cast<lua_Integer>(std::stoll(*v.pstr,nullptr,0));
		}
		catch(std::exception &) {
			return 0;
		}
	case Array:
	case Table:
	case CFunction:
	case LightUserData:
	case FileHandle:
	case Invalid:
		return 0;
	}
	return 0;
}

#ifdef HAVE_REF_QUALIFIERS

std::string LuaValue::toString() const & {
	return convertToString();
}

std::string LuaValue::toString() && {
	if(t==String) return std::move(*v.pstr);
	else return convertToString();
}

#else // not HAVE_REF_QUALIFIERS

std::string LuaValue::toString() const {
	return convertToString();
}

#endif

lua_CFunction LuaValue::toCFunction() const {
	if(t==Nil) return NULL;
	if(t==CFunction) return v.pclosure->pf;
	throw std::runtime_error("LuaValue is not a C function");
}

lua_CFunction LuaValue::toCFunction(std::vector<LuaValue> &upvalues) const {
	if(t==Nil) return NULL;
	if(t==CFunction) {
		upvalues=v.pclosure->upvalues;
		return v.pclosure->pf;
	}
	throw std::runtime_error("LuaValue is not a C function");
}

void *LuaValue::toLightUserData() const {
	if(t==Nil) return NULL;
	if(t==LightUserData) return v.plud;
	throw std::runtime_error("LuaValue is not light user data");
}

std::FILE *LuaValue::toFileHandle() const {
	if(t==Nil) return NULL;
	if(t==FileHandle) return v.fh;
	throw std::runtime_error("LuaValue is not a file handle");
}

std::vector<LuaValue> &LuaValue::newarray() {
	LuaValue temp;
	temp.t=Array;
	temp.v.parray=new std::vector<LuaValue>;
	swap(temp);
	return *v.parray;
}

std::vector<LuaValue> &LuaValue::array() {
	if(t!=Array) throw std::runtime_error("LuaValue is not an array");
	return *v.parray;
}

const std::vector<LuaValue> &LuaValue::array() const {
	if(t!=Array) throw std::runtime_error("LuaValue is not an array");
	return *v.parray;
}

std::map<LuaValue,LuaValue> &LuaValue::newtable() {
	LuaValue temp;
	temp.t=Table;
	temp.v.ptable=new LuaTable;
	swap(temp);
	return v.ptable->dict;
}

std::map<LuaValue,LuaValue> &LuaValue::table() {
	if(t!=Table) throw std::runtime_error("LuaValue is not a table");
	return v.ptable->dict;
}

const std::map<LuaValue,LuaValue> &LuaValue::table() const {
	if(t!=Table) throw std::runtime_error("LuaValue is not a table");
	return v.ptable->dict;
}

std::map<LuaValue,LuaValue> &LuaValue::metatable() {
	if(t!=Table) throw std::runtime_error("LuaValue is not a table");
	return v.ptable->meta;
}

const std::map<LuaValue,LuaValue> &LuaValue::metatable() const {
	if(t!=Table) throw std::runtime_error("LuaValue is not a table");
	return v.ptable->meta;
}

/*
 * Static members
 */

LuaValue LuaValue::invalid() {
	LuaValue temp;
	temp.t=Invalid;
	return temp;
}

std::string LuaValue::typeName(Type tt) {
	switch(tt) {
	case Nil:
		return "nil";
	case Boolean:
		return "boolean";
	case Number:
		return "number";
	case Integer:
		return "integer";
	case String:
		return "string";
	case Array:
		return "array";
	case Table:
		return "table";
	case CFunction:
		return "cfunction";
	case LightUserData:
		return "lightuserdata";
	case FileHandle:
		return "file";
	default:
		return "invalid";
	}
}

/*
 * Private members
 */

inline std::string LuaValue::convertToString() const {
	switch(t) {
	case Nil:
		return "nil";
	case Boolean:
		return v.b?"true":"false";
	case Number:
/*
 * Note 1: we use std::ostringstream instead of std::to_string() here,
 * because the latter always uses fixed notation, which is not generally
 * desirable (std::ostringstream uses either fixed or scientific notation
 * as appropriate, similar to the %g sprintf formatting specifier).
 * 
 * Note 2: we don't create std::ostringstream instance in the outer
 * scope since its constructor can be slow, see also
 * 23cb5b002b9aed22c78d6fdf26b800f0b06b18db
 * commit.
 */
		{
			std::ostringstream oss;
			oss.precision(14); // like in Lua
			oss<<v.d;
			return oss.str();
		}
	case Integer:
		return std::to_string(v.i);
	case String:
		return *v.pstr;
	case LightUserData:
	case FileHandle:
		{
			std::ostringstream oss;
			oss<<typeName()<<": 0x";
			oss<<std::hex<<std::setw(sizeof(void*)*2)<<std::setfill('0')<<std::right;
			oss<<reinterpret_cast<std::uintptr_t>(t==LightUserData?v.plud:v.fh);
			return oss.str();
		}
	default:
		return typeName();
	}
}
