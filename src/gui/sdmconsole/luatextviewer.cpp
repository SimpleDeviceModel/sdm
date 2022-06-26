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
 * This module provides an implementation of the LuaTextViewer
 * class members.
 */

#include "luatextviewer.h"

#include "fontutils.h"

using namespace std::placeholders;

LuaTextViewer::LuaTextViewer(QWidget *parent,const std::vector<LuaValue> &args):
	LuaModelessDialog(parent)
{
	if(args.size()>0) setWindowTitle(FString(args[0].toString()));
}

LuaTextViewer::~LuaTextViewer() {
/*
 * Note: althought the object will be unregistered in LuaCallbackObject::~LuaCallbackObject(),
 * we want to do it here to avoid the object being accessed from the worker thread
 * while the derived class is already destroyed, but the base class it not
 */
	detach();
}

LuaGUIObject::Invoker LuaTextViewer::enumerateGUIMethods(int i,std::string &strName,InvokeType &type) {
	switch(i) {
	case 0:
		strName="setfont";
		type=Async;
		return std::bind(&LuaTextViewer::LuaMethod_setfont,this,_1,_2);
	case 1:
		strName="settext";
		type=Async;
		return std::bind(&LuaTextViewer::LuaMethod_settext,this,_1,_2);
	case 2:
		strName="addtext";
		type=Async;
		return std::bind(&LuaTextViewer::LuaMethod_addtext,this,_1,_2);
	case 3:
		strName="setfile";
		type=Async;
		return std::bind(&LuaTextViewer::LuaMethod_setfile,this,_1,_2);
	case 4:
		strName="setwrap";
		type=Async;
		return std::bind(&LuaTextViewer::LuaMethod_setwrap,this,_1,_2);
	case 5:
		strName="clear";
		type=Async;
		return std::bind(&LuaTextViewer::LuaMethod_clear,this,_1,_2);
	default:
		return LuaModelessDialog::enumerateGUIMethods(i-6,strName,type);
	}
}

int LuaTextViewer::LuaMethod_setfont(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()<1||args.size()>3) throw std::runtime_error("setfont() method takes 1-3 arguments");
	const FString &str=args[0].toString();
	
	QFont f; // default application font
	if(str=="default");
	else if(str=="sans-serif") {
		f.setStyleHint(QFont::SansSerif);
		f.setFamily(f.defaultFamily());
	}
	else if(str=="serif") {
		f.setStyleHint(QFont::Serif);
		f.setFamily(f.defaultFamily());
	}
	else if(str=="monospace") {
		f=FontUtils::defaultFixedFont();
	}
	else f=QFont(str);
	
	if(args.size()>=2) f.setPointSizeF(args[1].toNumber());
	
	if(args.size()>=3) {
		auto const &style=args[2].toString();
		bool bold=false;
		bool italic=false;
		if(style=="regular");
		else if(style=="italic") italic=true;
		else if(style=="bold") bold=true;
		else if(style=="bold italic") {italic=true; bold=true;}
		else throw std::runtime_error("Unsupported style \""+style+"\": \"regular\", \"italic\", \"bold\" or \"bold italic\" expected");
		f.setBold(bold);
		f.setItalic(italic);
	}
	
	applyFont(f);
	
	return 0;
}

int LuaTextViewer::LuaMethod_settext(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=1) throw std::runtime_error("settext() method takes 1 argument");
	loadString(FString(args[0].toString()));
	return 0;
}

int LuaTextViewer::LuaMethod_addtext(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=1) throw std::runtime_error("addtext() method takes 1 argument");
	appendString(FString(args[0].toString()));
	return 0;
}

int LuaTextViewer::LuaMethod_setfile(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=1) throw std::runtime_error("setfile() method takes 1 argument");
	loadFile(FString(args[0].toString()));
	return 0;
}

int LuaTextViewer::LuaMethod_setwrap(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=1) throw std::runtime_error("setwrap() method takes 1 argument");
	wrap(args[0].toBoolean());
	return 0;
}

int LuaTextViewer::LuaMethod_clear(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=1) throw std::runtime_error("clear() method doesn't take arguments");
	clear();
	return 0;
}
