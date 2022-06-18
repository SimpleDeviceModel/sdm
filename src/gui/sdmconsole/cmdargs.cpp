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
 * This module provides an implementation of the CmdArgs struct.
 */

#include "cmdargs.h"
#include "u8eenv.h"

#include <stdexcept>

CmdArgs CmdArgs::global;

void CmdArgs::parseCommandLine(int argc,char *argv[]) {
	args=u8e::cmdArgs(argc,argv);
	for(std::size_t i=1;i<args.size();i++) {
		if(args[i]=="--alt-redirector") altRedirector=true;
		else if(args[i]=="--no-redirector") noRedirector=true;
		else if(args[i]=="--run") {
			batchMode=false;
			if(++i>=args.size()) throw std::runtime_error("Bad command line");
			scriptPos=i;
			scriptFile=args[i];
		}
		else if(args[i]=="--batch") {
			batchMode=true;
			if(++i>=args.size()) throw std::runtime_error("Bad command line");
			scriptPos=i;
			scriptFile=args[i];
		}
	}
}
