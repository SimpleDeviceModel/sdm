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
 * LuaServer is a small wrapper library integrating Lua interpreter
 * into a C++ program.
 *
 * This module implements a LuaConsole class.
 */

#include "luaconsole.h"
#include "luaserver.h"

#include "stringutils.h"

#include <stdexcept>
#include <utility>

LuaConsole::LuaConsole(LuaServer &l): lua(l),continuation(false),async(false) {}

void LuaConsole::completer(const LuaCallResult &res) {
	std::ostringstream ssOutput;
	
	if(res.success) {
		for(auto it=res.results.cbegin();it!=res.results.cend();it++) {
			if(it->type()==LuaValue::Table) {
				traverse_table(*it,ssOutput);
			}
			else ssOutput<<it->toString()<<std::endl;
		}
		
	}
	else {
		if(res.incomplete) continuation=true;
		else ssOutput<<res.errorMessage<<std::endl;
	}
	
	consoleOutput(ssOutput.str());
	
	if(!continuation) {
		ssChunk.str("");
		consolePrompt("> ");
	}
	else consolePrompt(">> ");
}

LuaServer::Completer LuaConsole::createCompletionFunctor() {
	return std::bind(&LuaConsole::completer,this,std::placeholders::_1);
}
	
void LuaConsole::consoleCommand(const std::string &cmd) {
	if(cmd.empty()) {
		if(continuation) consolePrompt(">> ");
		else consolePrompt("> ");
		return;
	}
	
	if(continuation) ssChunk<<std::endl;
	else ssChunk.str("");
	
	auto const &cleanCmd=StringUtils::cleanupString(cmd);
	
	if(cleanCmd=="quit") {
		consoleQuit();
		return;
	}
	else if(cleanCmd=="clear") {
		consoleClear();
		ssChunk.str("");
		consolePrompt("> ");
		return;
	}
	else ssChunk<<cmd;
	
	continuation=false;
	
	LuaCallResult res;
	
	if(async) {
		try {
			lua.executeChunkAsync(ssChunk.str(),"=input",createCompletionFunctor());
		}
		catch(std::exception &ex) {
			res.errorMessage=ex.what();
			completer(res);
		}
	}
	else {
		try {
			res=lua.executeChunk(ssChunk.str(),"=input");
		}
		catch(std::exception &ex) {
			res.errorMessage=ex.what();
		}
		completer(res);
	}
}

void LuaConsole::limitedprint(std::ostream &out,const std::string &str,std::size_t maxlen,bool pad) {
	std::string s=str;
// Ensure that there are no newlines
	std::size_t nl=s.find_first_of("\x0D\x0A");
	if(nl!=std::string::npos) s.replace(nl,std::string::npos,"..."); 

	std::size_t size=s.size();
	if(size<=maxlen) {
		out<<s;
		if(pad) for(std::size_t i=size;i<maxlen;i++) out<<' ';
	}
	else {
// Try to avoid breaking UTF-8 character
		std::size_t len;
		for(len=maxlen-3;len>maxlen-7&&len>0;len--) if((s[len]&0xC0)!=0x80) break;
		if((s[len]&0xC0)==0x80) len=maxlen-3;
		out<<s.substr(0,len)<<"...";
		if(pad) for(std::size_t i=len+3;i<maxlen;i++) out<<' ';
	}
}

void LuaConsole::traverse_table(const LuaValue &t,std::ostream &out,const std::string &prefix,bool recursive) {
	if(t.type()!=LuaValue::Table) out<<prefix<<"Not a table!"<<std::endl;
	if(t.size()==0) out<<prefix<<"[empty table]"<<std::endl;
	
	std::map<LuaValue,LuaValue>::const_iterator it;
	std::size_t maxkeylen=0;
	
// Obtain maximum key string length
	for(it=t.table().begin();it!=t.table().end();it++) {
		if(it->first.toString().size()>maxkeylen) maxkeylen=it->first.toString().size();
	}
	
// Not too large?
	if(maxkeylen>maxKeyLineSize) maxkeylen=maxKeyLineSize;
	
	for(it=t.table().begin();it!=t.table().end();it++) {
		out<<prefix;
		limitedprint(out,it->first.toString(),maxkeylen,true);
		out<<"    ";
		if(it->second.type()==LuaValue::String) { // add quotes
			out<<"\"";
			limitedprint(out,it->second.toString(),maxValLineSize-2);
			out<<"\"";
		}
		else limitedprint(out,it->second.toString(),maxValLineSize);
		out<<std::endl;
		
		if(it->second.type()==LuaValue::Table&&recursive) traverse_table(it->second,out,prefix+"    ");
	}
}
