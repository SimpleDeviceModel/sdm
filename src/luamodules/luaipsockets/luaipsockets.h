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
 * This is the main header for the "luaipsockets" Lua module that
 * provides Lua bindings for the IPSockets library.
 */

#ifndef LUAIPSOCKETS_H_INCLUDED
#define LUAIPSOCKETS_H_INCLUDED

#include "luacallbackobject.h"
#include "ipsocket.h"

// Lua module exported function

#ifdef _WIN32
	#define EXPORT extern "C" __declspec(dllexport)
#elif (__GNUC__>=4)
	#define EXPORT extern "C" __attribute__((__visibility__("default")))
#else
	#define EXPORT extern "C"
#endif

EXPORT int luaopen_luaipsockets(lua_State *L);

// Objects accessible from Lua

class LuaSocketLib : public LuaCallbackObject {
public:
	virtual std::string objectType() const override {return "IPSocketLib";}
protected:
	virtual std::function<int(LuaServer&)>
		enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) override;
	
	int LuaMethod_create(LuaServer &lua);
	int LuaMethod_gethostbyname(LuaServer &lua);
	int LuaMethod_list(LuaServer &lua);
};

class LuaSocket : public IPSocket,public LuaCallbackObject {
public:
	LuaSocket(Type t);
	LuaSocket(IPSocket &&s);
	virtual std::string objectType() const override {return "IPSocket";}
	
protected:
	virtual std::function<int(LuaServer&)>
		enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) override;
	
	int LuaMethod_close(LuaServer &lua);
	
	int LuaMethod_bind(LuaServer &lua);
	int LuaMethod_connect(LuaServer &lua);
	int LuaMethod_listen(LuaServer &lua);
	int LuaMethod_accept(LuaServer &lua);
	
	int LuaMethod_send(LuaServer &lua);
	int LuaMethod_sendall(LuaServer &lua);
	int LuaMethod_recv(LuaServer &lua);
	int LuaMethod_recvall(LuaServer &lua);
	int LuaMethod_wait(LuaServer &lua);
	
	int LuaMethod_shutdown(LuaServer &lua);
	
	int LuaMethod_setoption(LuaServer &lua);
	int LuaMethod_info(LuaServer &lua);
};

#endif
