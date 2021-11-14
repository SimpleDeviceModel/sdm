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
 * This header file defines Lua bindings for the TextViewer class.
 */

#ifndef LUATEXTVIEWER_H_INCLUDED
#define LUATEXTVIEWER_H_INCLUDED

#include "luamodelessdialog.h"
#include "textviewer.h"

class LuaTextViewer : public LuaModelessDialog<TextViewer> {
	Q_OBJECT
public:
	LuaTextViewer(QWidget *parent=nullptr,const std::vector<LuaValue> &args=std::vector<LuaValue>());
	virtual ~LuaTextViewer();
	
	virtual std::string objectType() const override {return "TextViewer";}
	
	virtual Invoker enumerateGUIMethods(int i,std::string &strName,InvokeType &type) override;
	int LuaMethod_setfont(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_settext(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_addtext(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_setfile(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_setwrap(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_clear(LuaServer &lua,const std::vector<LuaValue> &args);
};

#endif
