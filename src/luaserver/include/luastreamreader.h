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
 * LuaServer is a small wrapper library integrating Lua interpreter
 * into a C++ program.
 *
 * This header file defines the LuaStreamReader class which is used
 * to read Lua chunks.
 */

#ifndef LUASTREAMREADER_H_INCLUDED
#define LUASTREAMREADER_H_INCLUDED

#include "lua.hpp"

#include <iostream>
#include <string>
#include <vector>

class LuaStreamReader {
	std::istream &_src;
	std::vector<char> _buf;
	bool _file;
	bool _start=true;

public:
	explicit LuaStreamReader(std::istream &src,bool file=false,std::size_t bufSize=16384):
		_src(src),_buf(bufSize),_file(file) {}
	
	static const char *readerFunc(lua_State *,void *data,std::size_t *size);
private:
	const char *read(std::size_t *size);
};

#endif
