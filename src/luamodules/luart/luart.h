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
 * This is the main header for the "luart" Lua module that provides
 * Lua bindings for the Uart library.
 */

#ifndef LUART_H_INCLUDED
#define LUART_H_INCLUDED

#include "luacallbackobject.h"
#include "uart.h"

// Lua module exported function

#ifdef _WIN32
	#define EXPORT extern "C" __declspec(dllexport)
#elif (__GNUC__>=4)
	#define EXPORT extern "C" __attribute__((__visibility__("default")))
#else
	#define EXPORT extern "C"
#endif

EXPORT int luaopen_luart(lua_State *L);

// Objects accessible from Lua

class LuaUartLib : public LuaCallbackObject {
public:
	virtual std::string objectType() const override {return "UartLib";}
	
protected:
	virtual std::function<int(LuaServer&)> enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) override;
	int LuaMethod_open(LuaServer &lua);
	int LuaMethod_list(LuaServer &lua);
};

class LuaUart : public Uart,public LuaCallbackObject {
public:
	virtual std::string objectType() const override {return "Uart";}
	
protected:
	virtual std::function<int(LuaServer&)>
		enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) override;
	
	int LuaMethod_close(LuaServer &lua);
	
	int LuaMethod_setbaudrate(LuaServer &lua);
	int LuaMethod_setdatabits(LuaServer &lua);
	int LuaMethod_setstopbits(LuaServer &lua);
	int LuaMethod_setparity(LuaServer &lua);
	int LuaMethod_setflowcontrol(LuaServer &lua);
	
	int LuaMethod_write(LuaServer &lua);
	int LuaMethod_read(LuaServer &lua);
	
	int LuaMethod_setdtr(LuaServer &lua);
	int LuaMethod_getdsr(LuaServer &lua);
	int LuaMethod_setrts(LuaServer &lua);
	int LuaMethod_getcts(LuaServer &lua);
};

#endif
