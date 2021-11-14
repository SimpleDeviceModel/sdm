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
 * This header file declares a TextConsole class implementing
 * a simple iostream-based console for text applications.
 */

#ifndef TEXTCONSOLE_H_INCLUDED
#define TEXTCONSOLE_H_INCLUDED

#include "luaconsole.h"

class TextConsole : public LuaConsole {
	bool q;
	bool nl;
	
public:
	TextConsole(LuaServer &l);
	
	bool quit() {return q;}
	virtual void consoleCommand(const std::string &cmd);

	virtual void consoleQuit() {q=true;}
	virtual void consoleClear();
	virtual void consolePrompt(const std::string &prompt);
	virtual void consoleOutput(const std::string &output="");
};

#endif
