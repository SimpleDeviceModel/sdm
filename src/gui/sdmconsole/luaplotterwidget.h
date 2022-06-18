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
 * This header file defines Lua bindings for the PlotterWidget class.
 */

#ifndef LUAPLOTTERWIDGET_H_INCLUDED
#define LUAPLOTTERWIDGET_H_INCLUDED

#include "luamodelessdialog.h"
#include "plotterwidget.h"

class LuaPlotterWidget : public LuaModelessDialog<PlotterWidget> {
	Q_OBJECT
	
public:
	LuaPlotterWidget(QWidget *parent=nullptr,const std::vector<LuaValue> &args=std::vector<LuaValue>());
	virtual ~LuaPlotterWidget();
	
	virtual std::string objectType() const override {return "Plotter";}
	
protected:
	virtual Invoker enumerateGUIMethods(int i,std::string &strName,InvokeType &type) override;
	int LuaMethod_adddata(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_removedata(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_setmode(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_setoption(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_setlayeroption(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_zoom(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_addcursor(LuaServer &lua,const std::vector<LuaValue> &args);
};

#endif
