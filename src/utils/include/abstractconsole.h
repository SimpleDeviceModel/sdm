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
 * This header file declares an AbstractConsole class, which is
 * a base class for consoles with any function and representation.
 *
 * It is intended that AbstractConsole is used as a virtual base.
 * One derivative should implement function-related methods,
 * the other - representation-related methods.
 */

#ifndef ABSTRACTCONSOLE_H_INCLUDED
#define ABSTRACTCONSOLE_H_INCLUDED

#include <string>

class AbstractConsole {
public:
	virtual ~AbstractConsole() {}

/*
 * Function related members (for use by representation classes)
 */

// Process user command
	virtual void consoleCommand(const std::string &cmd)=0;

/*
 * Representation related members (for use by function classes)
 */

// Console or application shutdown
	virtual void consoleQuit()=0;

// Console buffer cleanup
	virtual void consoleClear()=0;

// Display prompt
	virtual void consolePrompt(const std::string &prompt)=0;

// Display output
	virtual void consoleOutput(const std::string &output="")=0;
	
};

#endif
