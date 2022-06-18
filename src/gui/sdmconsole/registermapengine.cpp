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
 * This module implements members of the RegisterMapEngine class.
 */

#include "registermapengine.h"

#include "registermaptable.h"
#include "registermapxml.h"
#include "luaserver.h"
#include "fstring.h"
#include "fruntime_error.h"

#include "sdmconfig.h"

#include <stdexcept>

using namespace std::placeholders;

RegisterMapEngine::RegisterMapEngine(LuaServer &l,QWidget *w): QTabWidget(w),_lua(l) {
	_handle=_lua.registerObject(*this);
	setMovable(true);
	tabBar()->hide();	
	pageNum=1;
	addPage();
}

RegisterMapEngine::~RegisterMapEngine() {
	_lua.unregisterObject(*this);
}

int RegisterMapEngine::pages() const {
	return count();
}

int RegisterMapEngine::addPage(const QString &name) {
	RegisterMapTable *rt=new RegisterMapTable;
	QString str=name.isEmpty()?QString(tr("Page %1")).arg(pageNum++):name;
	int i=addTab(rt,str);
	QObject::connect(rt,&RegisterMapTable::requestWriteReg,this,&RegisterMapEngine::requestWriteRegSlot);
	QObject::connect(rt,&RegisterMapTable::requestReadReg,this,&RegisterMapEngine::requestReadRegSlot);
	QObject::connect(rt,&RegisterMapTable::cellChanged,this,&RegisterMapEngine::selectionChanged);
	QObject::connect(rt,&RegisterMapTable::currentCellChanged,this,&RegisterMapEngine::selectionChanged);
	QObject::connect(rt,&RegisterMapTable::requestConfigDialog,this,&RegisterMapEngine::requestConfigDialogSlot);
	QObject::connect(rt,&RegisterMapTable::rowDataChanged,this,&RegisterMapEngine::rowDataChangedSlot);
	setCurrentIndex(i);
	insertRow(i,0,RegisterMap::RowData());
	if(pages()>1) {
		setTabsClosable(true);
		tabBar()->show();
	}
	return i;
}

void RegisterMapEngine::removePage(int i) {
	if(i<0||i>=pages()) throw fruntime_error(tr("RegisterMapEngine: page doesn't exist"));
	delete widget(i);
	if(pages()<=1) {
		setTabsClosable(false);
		tabBar()->hide();
	}
}

QString RegisterMapEngine::pageName(int i) const {
	return tabText(i);
}

void RegisterMapEngine::setPageName(int i,const QString &name) {
	if(i<0||i>=pages()) throw fruntime_error(tr("RegisterMapEngine: page doesn't exist"));
	setTabText(i,name);
}

int RegisterMapEngine::rows(int page) {
	if(page<0||page>=pages()) throw fruntime_error(tr("RegisterMapEngine: page doesn't exist"));
	return tableWidget(page)->rows();
}

void RegisterMapEngine::insertRow(int page,int i,const RegisterMap::RowData &data) {
	if(page<0||page>=pages()) throw fruntime_error(("RegisterMapEngine: page doesn't exist"));
	tableWidget(page)->insertRow(i,data);
	emit selectionChanged();
}

void RegisterMapEngine::removeRow(int page,int i) {
	if(page<0||page>=pages()) throw fruntime_error(("RegisterMapEngine: page doesn't exist"));
	tableWidget(page)->removeRow(i);
	emit selectionChanged();
}

const RegisterMap::RowData &RegisterMapEngine::rowData(int page,int i) {
	if(page<0||page>=pages()) throw fruntime_error(("RegisterMapEngine: page doesn't exist"));
	return tableWidget(page)->rowData(i);
}

void RegisterMapEngine::setRowData(int page,int i,const RegisterMap::RowData &data) {
	if(page<0||page>=pages()) throw fruntime_error(("RegisterMapEngine: page doesn't exist"));
	tableWidget(page)->setRowData(i,data);
}

int RegisterMapEngine::currentPage() const {
	return currentIndex();
}

int RegisterMapEngine::currentRow() const {
	auto page=currentIndex();
	if(page<0) return -1;
	return tableWidget(page)->currentRow();
}

void RegisterMapEngine::clear() {
	QTabWidget::clear();
	pageNum=1;
}

void RegisterMapEngine::setNumMode(RegisterMap::NumberMode mode) {
	_numMode=mode;
	for(int i=0;i<pages();i++) tableWidget(i)->setNumMode(_numMode);
}

void RegisterMapEngine::requestWriteRegSlot(int row) {
// detect page number and re-emit signal
	auto table=dynamic_cast<QWidget*>(sender());
	if(!table) return;
	emit requestWriteReg(indexOf(table),row);
}

void RegisterMapEngine::requestReadRegSlot(int row) {
// detect page number and re-emit signal
	auto table=dynamic_cast<QWidget*>(sender());
	if(!table) return;
	emit requestReadReg(indexOf(table),row);
}

void RegisterMapEngine::requestConfigDialogSlot(int row) {
// detect page number and re-emit signal
	auto table=dynamic_cast<QWidget*>(sender());
	if(!table) return;
	emit requestConfigDialog(indexOf(table),row);
}

void RegisterMapEngine::rowDataChangedSlot(int row) {
// detect page number and re-emit signal
	auto table=dynamic_cast<QWidget*>(sender());
	if(!table) return;
	emit rowDataChanged(indexOf(table),row);
}

/*
 * Private members
 */

RegisterMapTable *RegisterMapEngine::tableWidget(int index) const {
	if(index<0) index=currentIndex();
	RegisterMapTable *t=dynamic_cast<RegisterMapTable*>(widget(index));
	if(!t) throw fruntime_error(tr("Page not selected"));
	return t;
}

/*
 * Implement LuaGUIObject interface
 */

LuaGUIObject::Invoker
	RegisterMapEngine::enumerateGUIMethods(int i,std::string &strName,InvokeType &type) {
	
	switch(i) {
	case 0:
		strName="pages";
		return std::bind(&RegisterMapEngine::LuaMethod_pages,this,_1,_2);
	case 1:
		strName="addpage";
		return std::bind(&RegisterMapEngine::LuaMethod_addpage,this,_1,_2);
	case 2:
		strName="removepage";
		return std::bind(&RegisterMapEngine::LuaMethod_removepage,this,_1,_2);
	case 3:
		strName="pagename";
		return std::bind(&RegisterMapEngine::LuaMethod_pagename,this,_1,_2);
	case 4:
		strName="rows";
		return std::bind(&RegisterMapEngine::LuaMethod_rows,this,_1,_2);
	case 5:
		strName="insertrow";
		return std::bind(&RegisterMapEngine::LuaMethod_insertrow,this,_1,_2);
	case 6:
		strName="removerow";
		return std::bind(&RegisterMapEngine::LuaMethod_removerow,this,_1,_2);
	case 7:
		strName="rowdata";
		return std::bind(&RegisterMapEngine::LuaMethod_rowdata,this,_1,_2);
	case 8:
		strName="currentpage";
		return std::bind(&RegisterMapEngine::LuaMethod_currentpage,this,_1,_2);
	case 9:
		strName="currentrow";
		return std::bind(&RegisterMapEngine::LuaMethod_currentrow,this,_1,_2);
	case 10:
		strName="findrow";
		return std::bind(&RegisterMapEngine::LuaMethod_findrow,this,_1,_2);
	case 11:
		strName="clear";
		return std::bind(&RegisterMapEngine::LuaMethod_clear,this,_1,_2);
	case 12:
		strName="load";
		return std::bind(&RegisterMapEngine::LuaMethod_load,this,_1,_2);
	case 13:
		strName="save";
		return std::bind(&RegisterMapEngine::LuaMethod_save,this,_1,_2);
	default:
		return LuaGUIObject::Invoker();
	}
}

int RegisterMapEngine::LuaMethod_pages(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(!args.empty()) throw std::runtime_error("pages() method doesn't take arguments");
	lua.pushValue(static_cast<lua_Integer>(pages()));
	return 1;
}

int RegisterMapEngine::LuaMethod_addpage(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()>1) throw std::runtime_error("addpage() method takes 0-1 arguments");
	
	int i;
	if(args.empty()) i=addPage();
	else i=addPage(FString(args[0].toString()));
	lua.pushValue(static_cast<lua_Integer>(i+1));
	return 1;
}

int RegisterMapEngine::LuaMethod_removepage(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()>1) throw std::runtime_error("removepage() method takes 1 argument");
	removePage(static_cast<int>(args[0].toInteger())-1);
	return 0;
}

int RegisterMapEngine::LuaMethod_pagename(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()<1&&args.size()>2) throw std::runtime_error("pagename() method takes 1-2 arguments");
	const int page=static_cast<int>(args[0].toInteger()-1);
	lua.pushValue(FString(pageName(page)));
	if(args.size()==2) setPageName(page,FString(args[1].toString()));
	return 1;
}

int RegisterMapEngine::LuaMethod_rows(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=1) throw std::runtime_error("rows() method takes 1 argument");
	const int page=static_cast<int>(args[0].toInteger()-1);
	lua.pushValue(static_cast<lua_Integer>(rows(page)));
	return 1;
}

int RegisterMapEngine::LuaMethod_insertrow(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=3) throw std::runtime_error("insertrow() method takes 3 arguments");
	const int page=static_cast<int>(args[0].toInteger()-1);
	const int row=static_cast<int>(args[1].toInteger()-1);
	RegisterMap::RowData d;
	luaToRowData(args[2],d);
	insertRow(page,row,d);
	return 0;
}

int RegisterMapEngine::LuaMethod_removerow(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=2) throw std::runtime_error("removerow() method takes 2 arguments");
	const int page=static_cast<int>(args[0].toInteger()-1);
	const int row=static_cast<int>(args[1].toInteger()-1);
	removeRow(page,row);
	return 0;
}

int RegisterMapEngine::LuaMethod_rowdata(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()<2||args.size()>3) throw std::runtime_error("rowdata() method takes 2-3 arguments");
	const int page=static_cast<int>(args[0].toInteger()-1);
	const int row=static_cast<int>(args[1].toInteger()-1);
// Obtain current row data
	RegisterMap::RowData d=rowData(page,row);
	
	lua.pushValue(rowDataToLua(d));

// If requested, replace row data
	if(args.size()==3) {
		const LuaValue &t=args[2];
		luaToRowData(t,d);
		setRowData(page,row,d);
	}

	return 1;
}

int RegisterMapEngine::LuaMethod_currentpage(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(!args.empty()) throw std::runtime_error("currentpage() method doesn't take arguments");
	lua.pushValue(static_cast<lua_Integer>(currentPage()+1));
	return 1;
}

int RegisterMapEngine::LuaMethod_currentrow(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(!args.empty()) throw std::runtime_error("currentrow() method doesn't take arguments");
	lua.pushValue(static_cast<lua_Integer>(currentRow()+1));
	return 1;
}

int RegisterMapEngine::LuaMethod_findrow(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=2) throw std::runtime_error("findrow() takes 2 arguments");
	
	int foundPage=-1,foundRow=-1;
	const QString toFind=FString(args[0].toString());
	if(toFind.isEmpty()) throw std::runtime_error("findrow() first argument can't be empty");
	
	
	enum How {ByName,ById,ByAddr} how;
	
	if(args[1].toString()=="name") how=ByName;
	else if(args[1].toString()=="id") how=ById;
	else if(args[1].toString()=="addr") how=ByAddr;
	else throw std::runtime_error("Bad second argument: \"name\", \"id\" or \"addr\" expected");
	
	for(int page=0;page<pages();page++) {
		for(int row=0;row<rows(page);row++) {
			const RegisterMap::RowData &d=rowData(page,row);
			if(how==ByName) {
				if(d.name==toFind) {
					foundPage=page;
					foundRow=row;
					break;
				}
			}
			else if(how==ById) {
				if(d.id==toFind) {
					foundPage=page;
					foundRow=row;
					break;
				}
			}
			else {
				try {
					if(d.addr==RegisterMap::Number<sdm_addr_t>(toFind)) {
						foundPage=page;
						foundRow=row;
						break;
					}
				}
				catch(std::exception &) {}
			}
		}
	}
	
	if(foundPage<0||foundRow<0) {
		lua.pushValue(LuaValue());
		return 1;
	}
	
	lua.pushValue(rowDataToLua(rowData(foundPage,foundRow)));
	lua.pushValue(static_cast<lua_Integer>(foundPage+1));
	lua.pushValue(static_cast<lua_Integer>(foundRow+1));
	
	return 3;
}

int RegisterMapEngine::LuaMethod_clear(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(!args.empty()) throw std::runtime_error("clear() doesn't take arguments");
	clear();
	addPage();
	return 0;
}

int RegisterMapEngine::LuaMethod_load(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=1) throw std::runtime_error("load() requires 1 argument");
	RegisterMapXML xml(*this);
	Path path(args[0].toString());
	try {
		xml.load(FString(path.str()));
	}
	catch(std::exception &) {
// Try to resolve path relative to the SDM data directory
		if(!path.isAbsolute()) {
			path=Config::dataDir()+path;
			xml.load(FString(path.str()));
		}
		else throw;
	}
	return 0;
}

int RegisterMapEngine::LuaMethod_save(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=1) throw std::runtime_error("save() requires 1 argument");
	RegisterMapXML xml(*this);
	xml.save(FString(args[0].toString()));
	return 0;
}

LuaValue RegisterMapEngine::rowDataToLua(const RegisterMap::RowData &data) {
	LuaValue t;
	t.newtable();
	
	t.table()["name"]=FString(data.name);
	
	if(data.type==RegisterMap::Section) {
		t.table()["type"]="section";
	}
	else if(data.type==RegisterMap::Register) {
		t.table()["type"]="register";
		
		if(!data.id.isEmpty()) t.table()["id"]=FString(data.id);
		
		if(data.addr.valid()) t.table()["addr"]=static_cast<lua_Integer>(data.addr);
		if(data.data.valid()) t.table()["data"]=static_cast<lua_Integer>(data.data);
		
		if(data.widget==RegisterMap::LineEdit) t.table()["widget"]="lineedit";
		else if(data.widget==RegisterMap::DropDown) t.table()["widget"]="dropdown";
		else if(data.widget==RegisterMap::ComboBox) t.table()["widget"]="combobox";
		else t.table()["widget"]="pushbutton";
		
		t.table()["options"].newtable();
		for(std::size_t i=0;i<data.options.size();i++) {
			LuaValue option;
			option.newtable();
			option.table()["name"]=FString(data.options[i].first);
			option.table()["value"]=static_cast<lua_Integer>(data.options[i].second);
			t.table()["options"].table()[static_cast<lua_Integer>(i+1)]=option;
		}
	
		if(data.writeAction.use) t.table()["writeaction"]=FString(data.writeAction.script);
		if(data.readAction.use) t.table()["readaction"]=FString(data.readAction.script);
		
		t.table()["skipgroupwrite"]=data.skipGroupWrite;
		t.table()["skipgroupread"]=data.skipGroupRead;
	}
	else if(data.type==RegisterMap::Fifo||data.type==RegisterMap::Memory) {
		if(data.type==RegisterMap::Fifo) t.table()["type"]="fifo";
		else t.table()["type"]="memory";
		
		if(!data.id.isEmpty()) t.table()["id"]=FString(data.id);
		
		if(data.addr.valid()) t.table()["addr"]=static_cast<lua_Integer>(data.addr);
		
		if(data.fifo.usePreWrite) {
			t.table()["prewriteaddr"]=static_cast<lua_Integer>(data.fifo.preWriteAddr);
			t.table()["prewritedata"]=static_cast<lua_Integer>(data.fifo.preWriteData);
		}
		
		auto &array=t.table()["data"].newarray();
		array.reserve(data.fifo.data.size());
		for(auto value: data.fifo.data) array.push_back(static_cast<lua_Integer>(value));
		
		if(data.writeAction.use) t.table()["writeaction"]=FString(data.writeAction.script);
		if(data.readAction.use) t.table()["readaction"]=FString(data.readAction.script);
		
		t.table()["skipgroupwrite"]=data.skipGroupWrite;
		t.table()["skipgroupread"]=data.skipGroupRead;
	}
	
	return t;
}

void RegisterMapEngine::luaToRowData(const LuaValue &from,RegisterMap::RowData &to) {
	if(from.type()!=LuaValue::Table) throw std::runtime_error("Wrong third argument type (table expected)");
	std::map<LuaValue,LuaValue>::const_iterator it;
	
	if((it=from.table().find("name"))!=from.table().end()) to.name=FString(it->second.toString());
	
	if((it=from.table().find("type"))!=from.table().end()) {
		if(it->second.toString()=="section") to.type=RegisterMap::Section;
		else if(it->second.toString()=="register") to.type=RegisterMap::Register;
		else if(it->second.toString()=="fifo") to.type=RegisterMap::Fifo;
		else if(it->second.toString()=="memory") to.type=RegisterMap::Memory;
		else throw std::runtime_error("Wrong type field, \"section\", \"register\", \"fifo\" or \"memory\" expected");
	}
	
	if(to.type==RegisterMap::Register) {
		if((it=from.table().find("id"))!=from.table().end()) to.id=FString(it->second.toString());
		if((it=from.table().find("addr"))!=from.table().end())
			to.addr=static_cast<sdm_addr_t>(it->second.toInteger());
		
		if((it=from.table().find("widget"))!=from.table().end()) {
			if(it->second.toString()=="lineedit") to.widget=RegisterMap::LineEdit;
			else if(it->second.toString()=="dropdown") to.widget=RegisterMap::DropDown;
			else if(it->second.toString()=="combobox") to.widget=RegisterMap::ComboBox;
			else if(it->second.toString()=="pushbutton") to.widget=RegisterMap::Pushbutton;
			else throw std::runtime_error("Wrong widget field, \"lineedit\", \"dropdown\", \"combobox\" or \"pushbutton\" expected");
		}
		
		if((it=from.table().find("options"))!=from.table().end()) {
			const LuaValue &opts=it->second;
			if(opts.type()!=LuaValue::Table)
				throw std::runtime_error("Wrong options field type, table expected");
			to.options.clear();
			for(std::map<LuaValue,LuaValue>::const_iterator it=opts.table().begin();it!=opts.table().end();it++) {
				if(it->second.type()!=LuaValue::Table) throw std::runtime_error("Wrong option type, table expected");
				std::map<LuaValue,LuaValue>::const_iterator it2;
				FString name;
				RegisterMap::Number<sdm_reg_t> value;
				if((it2=it->second.table().find("name"))!=it->second.table().end())
					name=it2->second.toString();
				else throw std::runtime_error("No \"name\" field for an option");
				if((it2=it->second.table().find("value"))!=it->second.table().end())
					value=static_cast<sdm_reg_t>(it2->second.toInteger());
				else throw std::runtime_error("No \"value\" field for an option");
				to.options.emplace_back(name,value);
			}
		}
		
	// Fill data after options to ensure correct dropdown list state
		if((it=from.table().find("data"))!=from.table().end())
			to.data=static_cast<sdm_reg_t>(it->second.toInteger());
		
		if((it=from.table().find("writeaction"))!=from.table().end()) {
			to.writeAction.use=true;
			to.writeAction.script=FString(it->second.toString());
		}
		if((it=from.table().find("readaction"))!=from.table().end()) {
			to.readAction.use=true;
			to.readAction.script=FString(it->second.toString());
		}
		
		if((it=from.table().find("skipgroupwrite"))!=from.table().end())
			to.skipGroupWrite=it->second.toBoolean();
		if((it=from.table().find("skipgroupread"))!=from.table().end())
			to.skipGroupRead=it->second.toBoolean();
	}
	else if(to.type==RegisterMap::Fifo||to.type==RegisterMap::Memory) {
		bool hasPreWriteAddr=false,hasPreWriteData=false;
		if((it=from.table().find("id"))!=from.table().end()) to.id=FString(it->second.toString());
		if((it=from.table().find("addr"))!=from.table().end())
			to.addr=static_cast<sdm_addr_t>(it->second.toInteger());
		
		if((it=from.table().find("prewriteaddr"))!=from.table().end()) {
			to.fifo.preWriteAddr=static_cast<sdm_addr_t>(it->second.toInteger());
			hasPreWriteAddr=true;
		}
		if((it=from.table().find("prewritedata"))!=from.table().end()) {
			to.fifo.preWriteData=static_cast<sdm_reg_t>(it->second.toInteger());
			hasPreWriteData=true;
		}
		if(hasPreWriteAddr&&hasPreWriteData) to.fifo.usePreWrite=true;
		
		if((it=from.table().find("data"))!=from.table().end()) {
			if(it->second.type()!=LuaValue::Table) throw std::runtime_error("Wrong data field type, table expected");
			to.fifo.data.clear();
			to.fifo.data.reserve(it->second.size());
			for(auto pair: it->second.table()) to.fifo.data.push_back(static_cast<sdm_reg_t>(pair.second.toInteger()));
		}
			
		if((it=from.table().find("writeaction"))!=from.table().end()) {
			to.writeAction.use=true;
			to.writeAction.script=FString(it->second.toString());
		}
		if((it=from.table().find("readaction"))!=from.table().end()) {
			to.readAction.use=true;
			to.readAction.script=FString(it->second.toString());
		}
		
		if((it=from.table().find("skipgroupwrite"))!=from.table().end())
			to.skipGroupWrite=it->second.toBoolean();
		if((it=from.table().find("skipgroupread"))!=from.table().end())
			to.skipGroupRead=it->second.toBoolean();
	}
}
