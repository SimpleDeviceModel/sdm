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
 * This module implements a stackGuard() function which is used
 * to protect C++ code from longjmp() in Lua interpreter. It must
 * be implemented in a separate translation unit to prevent the
 * compiler from optimizing it away. See more detailed explanation
 * in luaserver.cpp.
 */

#include "luaserver.h"

// Note: LuaServer::stackGuard() is a static member of LuaServer

int LuaServer::stackGuard(lua_State *L) {
	int r=LuaServer::globalDispatcher(L);
	if(r>=0) return r;
	return lua_error(L);
}

// Note: LuaServer::hookStackGuard() is a static member of LuaServer

void LuaServer::hookStackGuard(lua_State *L,lua_Debug *ar) {
	int r=LuaServer::globalHook(L,ar);
	if(r<0) lua_error(L);
}
