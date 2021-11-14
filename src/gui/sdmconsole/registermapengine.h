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
 * This header file defines a class implementing tables in the
 * reigster map widget.
 */

#ifndef REGISTERMAPENGINE_H_INCLUDED
#define REGISTERMAPENGINE_H_INCLUDED

#include "registermaptypes.h"
#include "sdmtypes.h"
#include "luagui.h"

#include <QString>
#include <QTabWidget>

class RegisterMapTable;

class RegisterMapEngine : public QTabWidget,public LuaGUIObject {
	Q_OBJECT
	
	LuaServer &_lua;
	int pageNum;
	LuaValue _handle;
	RegisterMap::NumberMode _numMode;
	
public:
	RegisterMapEngine(LuaServer &l,QWidget *w=nullptr);
	virtual ~RegisterMapEngine();
	
	const LuaValue &handle() const {return _handle;}
	
	int pages() const;
	int addPage(const QString &name="");
	void removePage(int i);
	QString pageName(int i) const;
	void setPageName(int i,const QString &name);
	
	int rows(int page);
	void insertRow(int page,int i,const RegisterMap::RowData &data);
	void removeRow(int page,int i);
	const RegisterMap::RowData &rowData(int page,int i);
	void setRowData(int page,int i,const RegisterMap::RowData &data);
	
	int currentPage() const;
	int currentRow() const;
	
	void clear();
	
	RegisterMap::NumberMode numMode() const {return _numMode;}
	void setNumMode(RegisterMap::NumberMode mode);
	
	static LuaValue rowDataToLua(const RegisterMap::RowData &data);
	static void luaToRowData(const LuaValue &from,RegisterMap::RowData &to);
	
	virtual std::string objectType() const override {return "RegisterMap";}

public slots:
	void requestConfigDialogSlot(int row);
	void rowDataChangedSlot(int row);
	void requestWriteRegSlot(int row);
	void requestReadRegSlot(int row);

signals:
	void selectionChanged();
	void requestWriteReg(int page,int row);
	void requestReadReg(int page,int row);
	void requestConfigDialog(int page,int row);
	void rowDataChanged(int page,int row);

private:
	RegisterMapTable *tableWidget(int page) const;

// Lua members
	virtual Invoker enumerateGUIMethods(int i,std::string &strName,InvokeType &type) override;
	int LuaMethod_pages(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_addpage(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_removepage(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_pagename(LuaServer &lua,const std::vector<LuaValue> &args);
	
	int LuaMethod_rows(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_insertrow(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_removerow(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_rowdata(LuaServer &lua,const std::vector<LuaValue> &args);
	
	int LuaMethod_currentpage(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_currentrow(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_findrow(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_clear(LuaServer &lua,const std::vector<LuaValue> &args);
	
	int LuaMethod_load(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_save(LuaServer &lua,const std::vector<LuaValue> &args);
};

#endif
