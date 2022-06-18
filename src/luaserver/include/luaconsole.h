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
 * This module declares an abstract Lua console class for both
 * text-based and GUI-based consoles.
 */

#ifndef LUACONSOLE_H_INCLUDED
#define LUACONSOLE_H_INCLUDED

#include "abstractconsole.h"
#include "luaserver.h"

#include <iostream>
#include <sstream>

class LuaConsole : virtual public AbstractConsole {
	std::ostringstream ssChunk;
	LuaServer &lua;
	bool continuation;
	bool async;
	
	void completer(const LuaCallResult &res);
	
	static const int maxKeyLineSize=24;
	static const int maxValLineSize=48;

	static void limitedprint(std::ostream &out,const std::string &str,std::size_t maxlen,bool pad=false);
	static void traverse_table(const LuaValue &t,std::ostream &out,const std::string &prefix="",bool recursive=false);
protected:
	virtual LuaServer::Completer createCompletionFunctor();
public:
	LuaConsole(LuaServer &l);
	virtual ~LuaConsole() {}

	virtual void consoleCommand(const std::string &cmd);
	
	void setAsyncMode(bool b) {async=b;}
};

#endif
