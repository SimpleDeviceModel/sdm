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
 * This header file provides an implementation of the LuaFormDialog
 * class members.
 */

#include "luaformdialog.h"

using namespace std::placeholders;

LuaFormDialog::LuaFormDialog(QWidget *parent,const std::vector<LuaValue> &args):
	LuaModelessDialog(parent)
{
	if(args.size()>0) setLabelText(FString(args[0].toString()));
}

LuaFormDialog::~LuaFormDialog() {
/*
 * Note: althought the object will be unregistered in LuaCallbackObject::~LuaCallbackObject(),
 * we want to do it here to avoid the object being accessed from the worker thread
 * while the derived class is already destroyed, but the base class it not
 */
	detach();
}

LuaGUIObject::Invoker LuaFormDialog::enumerateGUIMethods(int i,std::string &strName,InvokeType &type) {
	switch(i) {
	case 0:
		strName="settext";
		return std::bind(&LuaFormDialog::LuaMethod_settext,this,_1,_2);
	case 1:
		strName="addtextoption";
		type=Async;
		return std::bind(&LuaFormDialog::LuaMethod_addtextoption,this,_1,_2);
	case 2:
		strName="addlistoption";
		type=Async;
		return std::bind(&LuaFormDialog::LuaMethod_addlistoption,this,_1,_2);
	case 3:
		strName="addfileoption";
		type=Async;
		return std::bind(&LuaFormDialog::LuaMethod_addfileoption,this,_1,_2);
	case 4:
		strName="getoption";
		return std::bind(&LuaFormDialog::LuaMethod_getoption,this,_1,_2);
	case 5:
		strName="exec";
		return std::bind(&LuaFormDialog::LuaMethod_exec,this,_1,_2);
	default:
		return LuaModelessDialog::enumerateGUIMethods(i-6,strName,type);
	}
}

int LuaFormDialog::LuaMethod_settext(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()>1) throw std::runtime_error("settext() method takes 0-1 arguments");
	lua.pushValue(LuaValue(FString(labelText())));
	if(args.size()==1) setLabelText(FString(args[0].toString()));
	return 1;
}

int LuaFormDialog::LuaMethod_addtextoption(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=1&&args.size()!=2) throw std::runtime_error("addtextoption() method takes 1-2 arguments");
	
	const FString &name=args[0].toString();
	FString value;
	
	if(args.size()>1) value=args[1].toString();
	
	addTextOption(name,value);
	
	return 0;
}

int LuaFormDialog::LuaMethod_addlistoption(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=2&&args.size()!=3) throw std::runtime_error("addlistoption() method takes 2-3 arguments");
	if(args[1].type()!=LuaValue::Table) throw std::runtime_error("Wrong 2nd argument type: table expected");
	
	std::vector<QString> options;
	for(auto it=args[1].table().cbegin();it!=args[1].table().cend();it++)
		options.emplace_back(FString(it->second.toString()));
	
	int defaultOption=0;
	if(args.size()>2) {
		if(args[2].type()==LuaValue::Number||args[2].type()==LuaValue::Integer) {
			defaultOption=static_cast<int>(args[2].toInteger())-1;
		}
		else {
			const FString &defName=args[2].toString();
			for(int i=0;i<static_cast<int>(options.size());i++) {
				if(options[i]==defName) {
					defaultOption=i;
					break;
				}
			}
		}
	}
	
	addDropDownOption(FString(args[0].toString()),options,defaultOption);
	
	return 0;
}

int LuaFormDialog::LuaMethod_addfileoption(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()<2||args.size()>4) throw std::runtime_error("addfileoption() method takes 2-4 arguments");
	
	const FString &name=args[0].toString();
	const FString &mode=args[1].toString();
	
	FileSelector::Mode m;
	if(mode=="open") m=FileSelector::Open;
	else if(mode=="save") m=FileSelector::Save;
	else if(mode=="dir") m=FileSelector::Dir;
	else throw std::runtime_error("Wrong second argument: \"open\", \"save\" or \"dir\" expected");
	
	FString filename;
	if(args.size()>2) filename=args[2].toString();
	
	FString filter;
	if(args.size()>3&&args[3].size()) filter=args[3].toString();
	else filter=tr("All files (*)");
	
	addFileOption(name,m,filter,filename);
	
	return 0;
}

int LuaFormDialog::LuaMethod_getoption(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=1) throw std::runtime_error("getoption() method takes 1 argument");
	
	int opt=-1;
	if(args[0].type()==LuaValue::Number||args[0].type()==LuaValue::Integer)
		opt=static_cast<int>(args[0].toInteger())-1;
	else {
		const QString &name=FString(args[0].toString());
		for(int i=0;i<options();i++) {
			if(optionName(i)==name) {
				opt=i;
				break;
			}
		}
		if(opt<0) throw std::runtime_error("No such option");
	}
	
	const QString &str=optionValue(opt);
	const int index=optionIndex(opt);
	
	lua.pushValue(FString(str));
	if(index<0) lua.pushValue(LuaValue());
	else lua.pushValue(static_cast<lua_Integer>(index)+1);
	
	return 2;
}

int LuaFormDialog::LuaMethod_exec(LuaServer &lua,const std::vector<LuaValue> &args) {
	int r=exec();
	lua.pushValue(r!=0);
	return 1;
}
