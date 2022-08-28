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
 * This module implements a TextConsole class which is used in
 * SDMHost interactive mode.
 */

#include "textconsole.h"
#include "u8eio.h"

using namespace u8e;

TextConsole::TextConsole(LuaServer &l): LuaConsole(l),q(false),nl(true) {
	utf8cerr()<<"Type \"quit\" to exit"<<endl;
	consolePrompt("> ");
}
		
void TextConsole::consoleCommand(const std::string &cmd) {
// Assume that LuaConsole::consoleCommand output will be terminated
// by a newline (we don't have any means to know for sure)
	nl=true;
	LuaConsole::consoleCommand(cmd);
}

void TextConsole::consolePrompt(const std::string &prompt) {
	if(!nl) utf8cout()<<std::endl;
	nl=false;
	_prompt=prompt;
}

void TextConsole::consoleOutput(const std::string &output) {
	if(output.empty()) return;
	utf8cout()<<output<<flush;
	if(output.back()=='\x0A') nl=true;
	else nl=false;
}

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Clear console buffer under Microsoft Windows
 * See https://support.microsoft.com/kb/99261/en-us
 */

void TextConsole::consoleClear() {
	HANDLE hConsole=GetStdHandle(STD_OUTPUT_HANDLE);
	if(hConsole==INVALID_HANDLE_VALUE) return;
	
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	BOOL b=GetConsoleScreenBufferInfo(hConsole,&csbi);
	if(!b) return;

// Fill console buffer with blanks
	DWORD dwConSize=csbi.dwSize.X*csbi.dwSize.Y;
	COORD home={0,0};
	DWORD dwCharsWritten;
	FillConsoleOutputCharacter(hConsole,
		' ',
		dwConSize,
		home,
		&dwCharsWritten);
	
// Reset to default character attributes: white on black
	WORD wAttributes=FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED;
	FillConsoleOutputAttribute(hConsole,
		wAttributes,
		dwConSize,
		home,
		&dwCharsWritten);
	
	SetConsoleTextAttribute(hConsole,wAttributes);

// Move cursor to (0,0)
	SetConsoleCursorPosition(hConsole,home);
	
	nl=true;
}

#else // not _WIN32

void TextConsole::consoleClear() {
	utf8cout()<<"\x1B[2J\x1B[H"<<flush; // clear screen, cursor to (0,0)
	nl=true;
}

#endif
