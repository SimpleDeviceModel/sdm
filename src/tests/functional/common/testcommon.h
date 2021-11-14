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
 * This header contains common definitions for the SDM functional
 * testing framework based on CTest.
 */


#ifndef TESTCOMMON_H_INCLUDED
#define TESTCOMMON_H_INCLUDED

#include "luaserver.h"

#ifdef _WIN32
	#define EXPORT extern "C" __declspec(dllexport)
#elif (__GNUC__>=4)
	#define EXPORT extern "C" __attribute__((__visibility__("default")))
#else
	#define EXPORT extern "C"
#endif

EXPORT int luaopen_testlib(lua_State *L);
int testmain(LuaServer &lua);

#endif
