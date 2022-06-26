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
 * This module provides an implementation of the LuaStreamReader class
 * members.
 */

#include "luastreamreader.h"

#include <cstring>
#include <limits>

const char *LuaStreamReader::readerFunc(lua_State *,void *data,std::size_t *size) {
	return static_cast<LuaStreamReader*>(data)->read(size);
}

const char *LuaStreamReader::read(std::size_t *size) {
	if(_start&&_file) {
// When reading from file, skip BOM and shebang (if any)
		char buf[3];

	// Ignore the so-called UTF-8 "BOM" (if any)
		_src.read(buf,3);
		if(!_src) {
			_src.clear();
			_src.seekg(0);
		}
		else if(std::memcmp(buf,"\xEF\xBB\xBF",3)) _src.seekg(0);
		
	// Ignore shebang (if any)
		auto pos=_src.tellg();
		_src.read(buf,2);
		if(!_src) {
			_src.clear();
			_src.seekg(pos);
		}
		else if(buf[0]!='#'||buf[1]!='!') _src.seekg(pos);
		else {
			_src.ignore(std::numeric_limits<std::streamsize>::max(),'\x0A');
			if(!_src.eof()) _src.seekg(-1,std::ios_base::cur); // for correct line number in Lua error reports
		}
	}
	
	_start=false;
	
	_src.read(_buf.data(),_buf.size());
	*size=static_cast<std::size_t>(_src.gcount());
	if(!*size) return nullptr;
	return _buf.data();
}
