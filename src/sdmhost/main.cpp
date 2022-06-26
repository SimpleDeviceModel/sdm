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
 * This module implements a simple, text-based host application
 * for SDM plugins.
 */

#include "sdmconfig.h"
#include "luabridge.h"
#include "luatextcodec.h"
#include "u8eio.h"
#include "u8efile.h"
#include "u8eenv.h"
#include "textconsole.h"
#include "luaiterator.h"

#include <exception>
#include <string>
#include <cstdlib>

using namespace u8e;

void displayusage(const std::string &strProgram) {
	utf8cerr()<<"Usage:"<<endl;
	utf8cerr()<<"\tInteractive mode:"<<endl;
	utf8cerr()<<"\t\t"<<strProgram<<endl<<endl;
	utf8cerr()<<"\tExecute a script, then exit:"<<endl;
	utf8cerr()<<"\t\t"<<strProgram<<" <filename> [script arguments]"<<endl<<endl;
	utf8cerr()<<"\tExecute a script, then enter interactive mode:"<<endl;
	utf8cerr()<<"\t\t"<<strProgram<<" -i <filename> [script arguments]"<<endl<<endl;
	utf8cerr()<<"\tDisplay this help message:"<<endl;
	utf8cerr()<<"\t\t"<<strProgram<<" <-h | --help>"<<endl<<endl;
}

/*
 * Entry point
 *
 * Define WinMain() when building headless host for Windows,
 * main() otherwise.
 */

#ifndef SDMHOST_NEEDS_WINMAIN
int main(int argc,char *argv[])
#else
// Note: u8e::cmdArgs doesn't use argc and argv on Windows
#define argc 0
#define argv nullptr
#include <windows.h>
int WINAPI WinMain(HINSTANCE,HINSTANCE,PSTR,int)
#endif

#ifdef NDEBUG
try
#endif
{
	utf8cerr()<<"sdmhost "<<Config::version()<<" ("<<Config::architecture()<<")"<<endl;
	utf8cerr()<<"Copyright (c) 2015-2022 Simple Device Model contributors"<<endl;
	utf8cerr()<<"This program comes with ABSOLUTELY NO WARRANTY."<<endl;
	utf8cerr()<<"This is free software, and you are welcome to redistribute it under certain conditions; see license.txt for details."<<endl;
	
	std::string strFileName;
	bool interactive;
	int firstLuaArgument=-1;
	
	auto const &args=cmdArgs(argc,argv);
	
	if(args.size()==1) {
		interactive=true;
	}
	else {
		if((args[1]=="-h")||(args[1]=="--help")) {
			displayusage(args[0]);
			return 0;
		}
		else if(args[1]=="-i") {
			if(args.size()<3) {
				displayusage(args[0]);
				return EXIT_FAILURE;
			}
			else {
				strFileName=args[2];
				interactive=true;
				firstLuaArgument=3;
			}
		}
		else {
			strFileName=args[1];
			interactive=false;
			firstLuaArgument=2;
		}
	}

	LuaServer lua;
	
	if(!lua.checkRuntime())
		utf8cerr()<<"Warning: main program and Lua interpreter use different instances of the Standard C library"<<endl;
	
	auto it=lua.getGlobalIterator("package");
	
	auto itPath=it.valueIterator("path");
	auto newPath=Config::luaModulePath();
#ifdef USING_SYSTEM_LUA
	if(!newPath.empty()&&newPath.back()!=';') newPath.push_back(';');
	itPath.setValue(newPath+itPath->second.toString());
#else
	itPath.setValue(newPath);
#endif
	
	auto itCPath=it.valueIterator("cpath");
	auto newCPath=Config::luaCModulePath(LUA_VERSION_MAJOR LUA_VERSION_MINOR);
#ifdef USING_SYSTEM_LUA
	if(!newCPath.empty()&&newCPath.back()!=';') newCPath.push_back(';');
	itCPath.setValue(newCPath+itCPath->second.toString());
#else
	itCPath.setValue(newCPath);
#endif
	
	LuaBridge doc(lua);
	doc.setInfoTag("host","sdmhost");
	doc.addPluginSearchPath(Config::pluginsDir().str());
	lua.setGlobal("sdm",doc.luaHandle());
	
	lua.setGlobal("codec",lua.addManagedObject(new LuaTextCodec));
	
	if(!strFileName.empty()) {
		Path filePath=Path(strFileName).toAbsolute();
		IFileStream in;
		
		in.open(filePath.str().c_str(),std::ios_base::in|std::ios_base::binary);
		if(!in) {
			utf8cerr()<<"Can't open file \""<<filePath.str()<<"\""<<endl;
			return EXIT_FAILURE;
		}
		
		LuaValue luaArg;
		luaArg.newtable();
// Script name is arg[0], everything before the script name has negative indices (like Lua standalone)
		if(firstLuaArgument<0) firstLuaArgument=static_cast<int>(args.size());
		for(std::size_t i=0;i<args.size();i++) {
			luaArg.table()[static_cast<lua_Integer>(i-firstLuaArgument+1)]=args[i];
		}
		lua.setGlobal("arg",luaArg);
		
		LuaStreamReader reader(in,true);
		LuaCallResult res=lua.executeChunk(reader,"@"+filePath.str());
		
		if(!res.success) {
			utf8cerr()<<res.errorMessage<<endl;
			return EXIT_FAILURE;
		}
	}
	
	if(interactive) { // interactive mode
		TextConsole console(lua);
		std::string strLine;
		
		while(!console.quit()&&std::getline(utf8cin(),strLine))
			console.consoleCommand(strLine);
	}
	
	return 0;
}
#ifdef NDEBUG
catch(std::exception &ex) {
	std::cerr<<"Fatal error: "<<ex.what()<<std::endl;
	return EXIT_FAILURE;
}
catch(...) {
	std::cerr<<"Fatal error: unexpected exception"<<std::endl;
	return EXIT_FAILURE;
}
#endif
