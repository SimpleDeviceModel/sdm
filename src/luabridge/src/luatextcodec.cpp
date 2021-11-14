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
 * This module implements members of the LuaTextCodec class.
 */

#include "luatextcodec.h"
#include "luaserver.h"
#include "u8eio.h"
#include "u8efile.h"
#include "dirutil.h"
#include "sdmconfig.h"

#include <stdexcept>
#include <sstream>

using namespace std::placeholders;

/*
 * LuaTextCodec members
 */

LuaTextCodec::LuaTextCodec(): toLocal(u8e::UTF8,u8e::LocalMB),fromLocal(u8e::LocalMB,u8e::UTF8) {}

std::function<int(LuaServer&)> LuaTextCodec::enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &) {
	switch(i) {
	case 0:
		strName="utf8tolocal";
		return std::bind(&LuaTextCodec::LuaMethod_utf8tolocal,this,_1);
	case 1:
		strName="localtoutf8";
		return std::bind(&LuaTextCodec::LuaMethod_localtoutf8,this,_1);
	case 2:
		strName="print";
		return std::bind(&LuaTextCodec::LuaMethod_print,this,_1);
	case 3:
		strName="write";
		return std::bind(&LuaTextCodec::LuaMethod_write,this,_1);
	case 4:
		strName="dofile";
		return std::bind(&LuaTextCodec::LuaMethod_dofile,this,_1);
	case 5:
		strName="open";
		return std::bind(&LuaTextCodec::LuaMethod_open,this,_1);
	case 6:
		strName="createcodec";
		return std::bind(&LuaTextCodec::LuaMethod_createcodec,this,_1);
	default:
		return std::function<int(LuaServer&)>();
	}
}

int LuaTextCodec::LuaMethod_utf8tolocal(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("utf8tolocal() method takes 1 argument");
	toLocal.reset();
	lua.pushValue(toLocal.transcode(lua.argv(0).toString()));
	return 1;
}

int LuaTextCodec::LuaMethod_localtoutf8(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("localtoutf8() method takes 1 argument");
	fromLocal.reset();
	lua.pushValue(fromLocal.transcode(lua.argv(0).toString()));
	return 1;
}

int LuaTextCodec::LuaMethod_print(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("print() method takes 1 argument");
	u8e::utf8cout()<<lua.argv(0).toString()<<u8e::endl;
	return 0;
}

int LuaTextCodec::LuaMethod_write(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("write() method takes 1 argument");
	u8e::utf8cout()<<lua.argv(0).toString()<<u8e::flush;
	return 0;
}

int LuaTextCodec::LuaMethod_dofile(LuaServer &lua) {
	if(lua.argc()>1) throw std::runtime_error("dofile() method takes 0-1 arguments");
	
	u8e::IFileStream inf;
	std::istream *in;
	std::string newChunkName;
	bool fileFlag;
	
	if(lua.argc()>0) {
		const std::string &filename=lua.argv(0).toString();
		Path path(filename);
		
		if(path.isAbsolute()) {
// Absolute path
			inf.open(path.str().c_str(),std::ios_base::in|std::ios_base::binary);
			if(!inf) throw std::runtime_error("Cannot open file \""+path.str()+"\"");
		}
		else {
			std::string locations="Locations tried:\n";
// Relative path: try to resolve relative to the directory containing the currently processed file
			auto const &chunkName=lua.currentChunkName();
			if(!chunkName.empty()&&chunkName[0]=='@') {
				path=chunkName.substr(1);
				path=path.toAbsolute().up()+filename;
				inf.open(path.str().c_str(),std::ios_base::in|std::ios_base::binary);
				locations+=path.str()+"\n";
			}
// Failing that, try SDM Lua modules directory (for supporting scripts)
			if(!inf||!inf.is_open()) {
				path=Config::luaModulesDir()+filename;
				inf.clear();
				inf.open(path.str().c_str(),std::ios_base::in|std::ios_base::binary);
				locations+=path.str()+"\n";
			}
// Failing that, try SDM scripts directory (for user-visible scripts)
			if(!inf||!inf.is_open()) {
				path=Config::scriptsDir()+filename;
				inf.clear();
				inf.open(path.str().c_str(),std::ios_base::in|std::ios_base::binary);
				locations+=path.str()+"\n";
			}
// Failing that, try current directory
			if(!inf||!inf.is_open()) {
				path=Path(filename).toAbsolute();
				inf.clear();
				inf.open(path.str().c_str(),std::ios_base::in|std::ios_base::binary);
				locations+=path.str();
				if(!inf) throw std::runtime_error("Cannot open file \""+filename+"\"\n"+locations);
			}
		}
		
		in=&inf;
		newChunkName="@"+path.str();
		fileFlag=true;
	}
	else { // no arguments - use standard input
		in=&std::cin;
		newChunkName="=stdin";
		fileFlag=false;
	}
	
	LuaStreamReader reader(*in,fileFlag);
	lua.clearstack(); // pop argument (if any)
	auto old=lua.autoClearStack();
	lua.setAutoClearStack(false); // retain values returned by the chunk
	auto luaResult=lua.executeChunk(reader,newChunkName);
	lua.setAutoClearStack(old);
	in->clear(); // to unset EOF on stdin
	if(!luaResult.success) {
		auto msg="Error in nested Lua call:\n"+luaResult.errorMessage;
		throw std::runtime_error(msg.c_str());
	}
	
	return static_cast<int>(luaResult.results.size());
}

int LuaTextCodec::LuaMethod_open(LuaServer &lua) {
	if(lua.argc()!=1&&lua.argc()!=2) throw std::runtime_error("open() method takes 1 or 2 arguments");
	
	if(!runtimeChecked) runtimeOk=lua.checkRuntime();
	if(!runtimeOk) throw std::runtime_error("Lua and main program use different standard C runtimes, open() method disabled");
	
	const std::string &path=lua.argv(0).toString();
	std::string mode="r";
	if(lua.argc()>1) mode=lua.argv(1).toString();
	
	auto fp=u8e::cfopen(path.c_str(),mode.c_str());
	if(!fp) {
		lua.pushValue(LuaValue());
		lua.pushValue("Cannot open file \""+path+"\"");
		return 2;
	}
	
	lua.pushValue(fp);
	
	return 1;
}

int LuaTextCodec::LuaMethod_createcodec(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("createcodec() method takes 1 argument");
	
// Parse encoding string
	auto str=lua.argv(0).toString();
	u8e::Encoding enc;
	
	for(auto it=str.begin();it!=str.end();) { // remove hyphens and underscores
		if(*it=='-'||*it=='_') it=str.erase(it);
		else ++it;
	}
	
	if(str=="local"||str=="ansi") enc=u8e::LocalMB;
	else if(str=="oem") enc=u8e::OEM;
	else if(str=="wchart") enc=u8e::WCharT;
	else if(str=="utf8") enc=u8e::UTF8;
	else if(str=="utf16") enc=u8e::UTF16;
	else if(str=="utf16le") enc=u8e::UTF16LE;
	else if(str=="utf16be") enc=u8e::UTF16BE;
	else if(str=="iso88591") enc=u8e::ISO_8859_1;
	else if(str=="iso88592") enc=u8e::ISO_8859_2;
	else if(str=="iso88593") enc=u8e::ISO_8859_3;
	else if(str=="iso88594") enc=u8e::ISO_8859_4;
	else if(str=="iso88595") enc=u8e::ISO_8859_5;
	else if(str=="iso88596") enc=u8e::ISO_8859_6;
	else if(str=="iso88597") enc=u8e::ISO_8859_7;
	else if(str=="iso88598") enc=u8e::ISO_8859_8;
	else if(str=="iso88599") enc=u8e::ISO_8859_9;
	else if(str=="iso885913") enc=u8e::ISO_8859_13;
	else if(str=="iso885915") enc=u8e::ISO_8859_15;
	else if(str=="windows1250") enc=u8e::Windows1250;
	else if(str=="windows1251") enc=u8e::Windows1251;
	else if(str=="windows1252") enc=u8e::Windows1252;
	else if(str=="windows1253") enc=u8e::Windows1253;
	else if(str=="windows1254") enc=u8e::Windows1254;
	else if(str=="windows1255") enc=u8e::Windows1255;
	else if(str=="windows1256") enc=u8e::Windows1256;
	else if(str=="windows1257") enc=u8e::Windows1257;
	else if(str=="windows1258") enc=u8e::Windows1258;
	else if(str=="cp437") enc=u8e::CP437;
	else if(str=="cp850") enc=u8e::CP850;
	else if(str=="cp866") enc=u8e::CP866;
	else if(str=="koi8r") enc=u8e::KOI8R;
	else if(str=="koi8u") enc=u8e::KOI8U;
	else if(str=="gb2312") enc=u8e::GB2312;
	else if(str=="gb18030") enc=u8e::GB18030;
	else if(str=="big5") enc=u8e::BIG5;
	else if(str=="shiftjis") enc=u8e::ShiftJIS;
	else if(str=="eucjp") enc=u8e::EUCJP;
	else if(str=="euckr") enc=u8e::EUCKR;
	else throw std::runtime_error("Unrecognized encoding: \""+str+"\"");
	
	lua.pushValue(lua.addManagedObject(new LuaCodecFSM(enc)));
	
	return 1;
}

/*
 * LuaCodecFSM members
 */

LuaCodecFSM::LuaCodecFSM(u8e::Encoding enc):
	fromUtf8(u8e::UTF8,enc),toUtf8(enc,u8e::UTF8) {}

std::function<int(LuaServer&)> LuaCodecFSM::enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &) {
	switch(i) {
	case 0:
		strName="close";
		return std::bind(&LuaCodecFSM::LuaMethod_close,this,_1);
	case 1:
		strName="fromutf8";
		return std::bind(&LuaCodecFSM::LuaMethod_fromutf8,this,_1);
	case 2:
		strName="toutf8";
		return std::bind(&LuaCodecFSM::LuaMethod_toutf8,this,_1);
	case 3:
		strName="reset";
		return std::bind(&LuaCodecFSM::LuaMethod_reset,this,_1);
	default:
		return std::function<int(LuaServer&)>();
	}
}

int LuaCodecFSM::LuaMethod_close(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("close() method doesn't take arguments");
	delete this;
	return 0;
}

int LuaCodecFSM::LuaMethod_fromutf8(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("fromutf8() method takes 1 argument");
	lua.pushValue(fromUtf8.transcode(lua.argv(0).toString()));
	return 1;
}

int LuaCodecFSM::LuaMethod_toutf8(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("toutf8() method takes 1 argument");
	lua.pushValue(toUtf8.transcode(lua.argv(0).toString()));
	return 1;
}

int LuaCodecFSM::LuaMethod_reset(LuaServer &lua) {
	if(lua.argc()>1) throw std::runtime_error("reset() method takes 0-1 arguments");
	std::string what="both";
	if(lua.argc()>=2) what=lua.argv(1).toString();
	if(what=="fromutf8"||what=="both") fromUtf8.reset();
	if(what=="toutf8"||what=="both") toUtf8.reset();
	return 0;
}
