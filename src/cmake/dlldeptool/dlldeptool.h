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

#ifndef DLLDEPTOOL_H_INCLUDED
#define DLLDEPTOOL_H_INCLUDED

enum PEType {unknown,pe32,pe64};

#define pestr(x) ((x==pe64)?"PE64":"PE32")

enum ModuleValidation {nodecision,forbid,force};

#endif
