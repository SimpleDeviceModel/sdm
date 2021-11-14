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
 * This header file defines a server object providing the ability
 * to create certain simple Qt dialogs from Lua.
 */

#ifndef LUADIALOGSERVER_H_INCLUDED
#define LUADIALOGSERVER_H_INCLUDED

#include "luagui.h"
#include "luamodelessdialog.h"

#include <QProgressDialog>

#include <atomic>

class LuaDialogServer : public LuaGUIObject {
public:
	virtual std::string objectType() const override {return "DialogServer";}
private:
// Lua methods
	virtual Invoker enumerateGUIMethods(int i,std::string &strName,InvokeType &type) override;
	int LuaMethod_screen(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_messagebox(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_inputdialog(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_filedialog(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_createdialog(LuaServer &lua,const std::vector<LuaValue> &args);
};

class LuaProgressDialog : public LuaModelessDialog<QProgressDialog> {
	Q_OBJECT
	
	std::atomic<bool> _canceled {false};
public:
	explicit LuaProgressDialog(QWidget *p=NULL,const std::vector<LuaValue> &args=std::vector<LuaValue>());
	virtual ~LuaProgressDialog();
	
	virtual std::string objectType() const override {return "ProgressDialog";}
	
private:
// Lua methods
	virtual Invoker enumerateGUIMethods(int i,std::string &strName,InvokeType &type) override;
	int LuaMethod_setvalue(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_settext(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_setrange(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_reset(LuaServer &lua,const std::vector<LuaValue> &args);
	
	virtual std::function<int(LuaServer&)> enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) override;
	int LuaMethod_canceled(LuaServer &lua);
private slots:
	void notifyCancel();
};

#endif
