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
 * This header file defines the LuaValue class which is used to
 * represent Lua values in C++. The following Lua types are
 * supported:
 *         nil
 *         boolean
 *         number (i.e. double)
 *         integer
 *         string
 *         table (keys and values must also be of supported types)
 *         cfunction (more precisely, so-called "C Closure")
 *         lightuserdata (i.e. pointer)
 * 
 * Additional types (with no direct equivalent in Lua):
 *         invalid:    used for values that can't or won't be represented.
 *         array:      a simple array designed for performance. Represented
 *                     as std::vector<LuaValue>. Keys must be sequential
 *                     integers starting with 1 (per Lua custom).
 *         filehandle: a special case of userdata for file input/output
 *                     supported by the standard Lua "io" library
 */

#ifndef LUAVALUE_H_INCLUDED
#define LUAVALUE_H_INCLUDED

#include "lua.hpp"

#include <string>
#include <map>
#include <vector>
#include <cstdio>
#include <utility>

#ifdef HAVE_NOEXCEPT
	#define LUAVALUE_NOEXCEPT noexcept
#else
	#define LUAVALUE_NOEXCEPT throw()
#endif

class LuaValue {
public:
	enum Type {Nil,Boolean,Number,Integer,String,Array,Table,CFunction,LightUserData,FileHandle,Invalid};
	
private:
	struct LuaCClosure;
	struct LuaTable;
	union LuaValueContent {
		bool b;
		lua_Number d;
		lua_Integer i;
		std::string *pstr;
		std::vector<LuaValue> *parray;
		LuaTable *ptable;
		LuaCClosure *pclosure;
		void *plud;
		std::FILE *fh;
	};

	LuaValueContent v;
	Type t;
	
public:
	LuaValue(): t(Nil) {}
	LuaValue(bool bb): t(Boolean) {v.b=bb;}
	LuaValue(lua_Number dd): t(Number) {v.d=dd;}
	LuaValue(lua_Integer li): t(Integer) {v.i=li;}
	LuaValue(const std::string &s);
	LuaValue(std::string &&s);
	LuaValue(const char *sz);
	LuaValue(const char *sz,std::size_t len);
	LuaValue(const lua_CFunction p);
	LuaValue(const lua_CFunction p, const std::vector<LuaValue> &upvalues);
	LuaValue(void *p);
	LuaValue(std::FILE *f): t(FileHandle) {v.fh=f;}
	
	LuaValue(const LuaValue &orig);
	LuaValue(LuaValue &&orig) LUAVALUE_NOEXCEPT: t(Nil) {swap(orig);}
	
	~LuaValue();
	
	LuaValue &operator=(const LuaValue &right);
	LuaValue &operator=(LuaValue &&right) LUAVALUE_NOEXCEPT {
		swap(right);
		return *this;
	}
	
	void swap(LuaValue &other) LUAVALUE_NOEXCEPT {
		std::swap(v,other.v);
		std::swap(t,other.t);
	}
	
	Type type() const {return t;}
	std::string typeName() const;
	
	LuaValue &clear();
	
// for std::map, all non-equal values must be strictly ordered
	int compare(const LuaValue &right) const;
	
	bool operator==(const LuaValue &right) const {return (compare(right)==0);}
	bool operator!=(const LuaValue &right) const {return (compare(right)!=0);}
	bool operator<(const LuaValue &right) const {return (compare(right)<0);}
	bool operator>(const LuaValue &right) const {return (compare(right)>0);}
	bool operator<=(const LuaValue &right) const {return (compare(right)<=0);}
	bool operator>=(const LuaValue &right) const {return (compare(right)>=0);}
	
	std::size_t size() const;

	bool toBoolean() const;
	lua_Number toNumber() const;
	lua_Integer toInteger() const;
#ifdef HAVE_REF_QUALIFIERS
	std::string toString() const &;
	std::string toString() &&;
#else
	std::string toString() const;
#endif
	lua_CFunction toCFunction() const;
	lua_CFunction toCFunction(std::vector<LuaValue> &upvalues) const;
	void *toLightUserData() const;
	std::FILE *toFileHandle() const;
	
	std::vector<LuaValue> &newarray();
	std::vector<LuaValue> &array();
	const std::vector<LuaValue> &array() const;
	
	std::map<LuaValue,LuaValue> &newtable();
	std::map<LuaValue,LuaValue> &table();
	const std::map<LuaValue,LuaValue> &table() const;
	std::map<LuaValue,LuaValue> &metatable();
	const std::map<LuaValue,LuaValue> &metatable() const;
	
	static LuaValue invalid();
	static std::string typeName(Type tt);
	
private:
	std::string convertToString() const;
};

struct LuaValue::LuaCClosure {
	lua_CFunction pf;
	std::vector<LuaValue> upvalues;
};

struct LuaValue::LuaTable {
	std::map<LuaValue,LuaValue> dict;
	std::map<LuaValue,LuaValue> meta;
};

inline void swap(LuaValue &a,LuaValue &b) LUAVALUE_NOEXCEPT {
	a.swap(b);
}

namespace std {
	template <> inline void swap<LuaValue>(LuaValue &a,LuaValue &b) LUAVALUE_NOEXCEPT {
		a.swap(b);
	}
}

#endif
