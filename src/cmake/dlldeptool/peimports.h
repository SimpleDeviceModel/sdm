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
 * DllDepTool is a tool for Windows deployment automation.
 * It scans the executable modules (EXE/DLL) and copies all the
 * DLLs required to run them.
 */

#ifndef PEIMPORTS_H_INCLUDED
#define PEIMPORTS_H_INCLUDED

#include "dlldeptool.h"

#include <iostream>
#include <vector>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef unsigned long offset_int;

class PEImage {
	std::vector<IMAGE_SECTION_HEADER> secheaders;
	PEType type;
	std::vector<std::string> dlls;
	
	offset_int rvaToOffset(offset_int rva);
	offset_int readWord(std::istream &in,offset_int offset);
	offset_int readDword(std::istream &in,offset_int offset);
	void readData(std::istream &in,offset_int offset,char *buf,std::size_t n);
public:
	PEImage() {}
	PEImage(const std::string &strFileName) {load(strFileName);}
	
	int load(const std::string &strFileName);
	std::vector<std::string> getDlls() {return dlls;}
	PEType getArch() {return type;}
};

#endif
