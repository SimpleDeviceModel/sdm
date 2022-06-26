/*
 * Copyright (c) 2015-2022 Simple Device Model contributors
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
 * This is the main header for the "luadfic" Lua module that provides
 * Lua bindings for the Dynamic Function Interface Compiler library.
 */

#ifndef LUADFIC_H_INCLUDED
#define LUADFIC_H_INCLUDED

#include "dfic.h"
#include "compoundtype.h"
#include "arraybuffer.h"

#include "luacallbackobject.h"
#include "loadablemodule.h"

#include <memory>

// Lua module exported function

#ifdef _WIN32
	#define EXPORT extern "C" __declspec(dllexport)
#elif (__GNUC__>=4)
	#define EXPORT extern "C" __attribute__((__visibility__("default")))
#else
	#define EXPORT extern "C"
#endif

EXPORT int luaopen_luadfic(lua_State *L);

struct Parameter {
	std::unique_ptr<AbstractArrayBuffer> buf;
	CompoundType type;
	
	Parameter(CompoundType t): type(t) {}

// Note: MSVC 2013 isn't able to generate implicit move constructors
	Parameter(Parameter &&orig): buf(std::move(orig.buf)),type(orig.type) {}
	Parameter &operator=(Parameter &&other) {
		type=other.type;
		buf=std::move(other.buf);
		return *this;
	}
};

// Objects accessible from Lua

class LuaDficLib : public LuaCallbackObject {
public:
	virtual std::string objectType() const override {return "Dfic";}
protected:
	virtual std::function<int(LuaServer&)>
		enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) override;
	int LuaMethod_import(LuaServer &lua);
	int LuaMethod_buffer(LuaServer &lua);
	int LuaMethod_info(LuaServer &lua);
	int LuaMethod_typesize(LuaServer &lua);
	int LuaMethod_int2ptr(LuaServer &lua);
	int LuaMethod_ptr2int(LuaServer &lua);
};

class LuaDfic : public LuaCallbackObject {
	LoadableModule _module;
	Dfic::Interface _interface;
	std::vector<Parameter> _params;
	CompoundType _returnType;

public:
	LuaDfic(const std::string &moduleName,const std::string &symbolName,const std::string &prototype);
	
	virtual std::string objectType() const override {return "DficFunction";}
	
protected:
	virtual std::function<int(LuaServer&)>
		enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) override;
	
	int LuaMethod_close(LuaServer &lua);
	int LuaMethod_invoke(LuaServer &lua);
	int LuaMethod_meta_call(LuaServer &lua);

private:
	int invoke(LuaServer &lua,bool meta);
	std::vector<std::string> listDecoratedNames(const std::string &name) const;
};

class LuaBuffer : public LuaCallbackObject {
	char *_ptr=nullptr;
	std::size_t _size=0;
	bool _own=false;
	
public:
	LuaBuffer(std::size_t s); // allocate buffer
	LuaBuffer(void *p,std::size_t s); // attach to existing buffer
	LuaBuffer(const LuaBuffer &)=delete;
	LuaBuffer(LuaBuffer &&)=delete;
	virtual ~LuaBuffer();
	
	LuaBuffer &operator=(const LuaBuffer &)=delete;
	LuaBuffer &operator=(LuaBuffer &&)=delete;
	
	virtual std::string objectType() const override {return "DficBuffer";}
	
	char *ptr() {return _ptr;}
	const char *ptr() const {return _ptr;}

protected:
	virtual std::function<int(LuaServer&)>
		enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) override;
	
	int LuaMethod_close(LuaServer &lua);
	int LuaMethod_ptr(LuaServer &lua);
	int LuaMethod_write(LuaServer &lua);
	int LuaMethod_read(LuaServer &lua);
	int LuaMethod_resize(LuaServer &lua);
	int LuaMethod_meta_len(LuaServer &lua);
	int LuaMethod_meta_index(LuaServer &lua);
	int LuaMethod_meta_newindex(LuaServer &lua);
};

#endif
