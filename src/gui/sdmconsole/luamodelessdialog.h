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
 * This header file defines a base class template for modeless Qt
 * dialogs accessible from Lua. It is intended to be subclassed
 * in the following way:
 * 
 * 	class MyDialog : public LuaModelessDialog<QDialog> {};
 * 
 * where "QDialog" can be replaced with any class derived from
 * QDialog.
 * 
 * LuaModelessDialog::enumerateGUIMethods() should be invoked from
 * derived classes as appropriate.
 */

#ifndef LUAMODELESSDIALOG_H_INCLUDED
#define LUAMODELESSDIALOG_H_INCLUDED

#include "luagui.h"
#include "luaserver.h"
#include "fstring.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QDockWidget>

#include <stdexcept>
#include <utility>

template <typename T> class LuaModelessDialog : public T,public LuaGUIObject {
public:
	template <typename... Args> explicit LuaModelessDialog(Args&&... args):
		T(std::forward<Args>(args)...)
	{
		T::setWindowFlags(T::windowFlags()&~Qt::WindowContextHelpButtonHint);
	}

protected:
	virtual Invoker enumerateGUIMethods(int i,std::string &strName,InvokeType &type);
	int LuaMethod_show(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_close(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_settitle(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_resize(LuaServer &lua,const std::vector<LuaValue> &args);
	int LuaMethod_move(LuaServer &lua,const std::vector<LuaValue> &args);

private:
	QWidget *rootWidget();
};

template <typename T>
	LuaGUIObject::Invoker
		LuaModelessDialog<T>::enumerateGUIMethods(int i,std::string &strName,InvokeType &type) {
	
	switch(i) {
	case 0:
		strName="show";
		return std::bind(&LuaModelessDialog::LuaMethod_show,this,std::placeholders::_1,std::placeholders::_2);
	case 1:
		strName="close";
		return std::bind(&LuaModelessDialog::LuaMethod_close,this,std::placeholders::_1,std::placeholders::_2);
	case 2:
		strName="settitle";
		return std::bind(&LuaModelessDialog::LuaMethod_settitle,this,std::placeholders::_1,std::placeholders::_2);
	case 3:
		strName="resize";
		return std::bind(&LuaModelessDialog::LuaMethod_resize,this,std::placeholders::_1,std::placeholders::_2);
	case 4:
		strName="move";
		return std::bind(&LuaModelessDialog::LuaMethod_move,this,std::placeholders::_1,std::placeholders::_2);
	default:
		return Invoker();
	}
}

/*
 * show(visible)
 * 
 * "visible" is an optional boolean argument. Passing "true" value
 * shows the dialog, passing "false" hides it. When invoked without
 * an argument, dialog visibility remains unchanged.
 * 
 * Returns dialog visibility state prior to change by this method.
 */

template <typename T> int LuaModelessDialog<T>::LuaMethod_show(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()>1) throw std::runtime_error("show() method requires at least 1 argument");
	lua.pushValue(rootWidget()->isVisible());
	if(args.size()==1) rootWidget()->setVisible(args[0].toBoolean());
	return 1;
}

/*
 * close()
 * 
 * Destroys the dialog object, freeing all resources. Subsequent
 * attempts to invoke its methods will result in error being raised.
 * 
 * Note: container (if set) is not destroyed.
 */

template <typename T> int LuaModelessDialog<T>::LuaMethod_close(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=0) throw std::runtime_error("close() method doesn't take arguments");
	if(owner()) {
		enableUnsafeDestruction(true); // enable unregistration while callback is in progress
		delete this;
	}
	return 0;
}

/*
 * settitle(title)
 * 
 * "title" is an optional argument. When it is specified, sets
 * the dialog window title. When invoked without arguments,
 * dialog title is unchanged.
 * 
 * Returns the dialog window title prior to change by this method.
 */

template <typename T> int LuaModelessDialog<T>::LuaMethod_settitle(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()>1) throw std::runtime_error("settitle() method requires at least 1 argument");
	lua.pushValue(FString(rootWidget()->windowTitle()));
	if(args.size()==1) rootWidget()->setWindowTitle(FString(args[0].toString()));
	return 1;
}

/*
 * resize()
 * resize(width,height)
 * 
 * Resizes the dialog window. When invoked without arguments, the dialog
 * window size is unchanged.
 * 
 * Returns the dialog window size prior to change by this method (as
 * two return values: width, height).
 */

template <typename T> int LuaModelessDialog<T>::LuaMethod_resize(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=2&&args.size()!=0) throw std::runtime_error("resize() method takes either 0 or 2 arguments");
	lua.pushValue(static_cast<lua_Integer>(rootWidget()->width()));
	lua.pushValue(static_cast<lua_Integer>(rootWidget()->height()));
	if(args.size()==2) rootWidget()->resize(static_cast<int>(args[0].toInteger()),static_cast<int>(args[1].toInteger()));
	return 2;
}

/*
 * move()
 * move(left,top)
 * move("center")
 * 
 * When invoked in the first form (without arguments), the dialog
 * window position is unchanged. When invoked in the second form,
 * moves the dialog window relative to the desktop. The third form
 * centers the dialog window.
 * 
 * Returns the dialog window position prior to change by this method
 * (as two return values: left, top).
 */

template <typename T> int LuaModelessDialog<T>::LuaMethod_move(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()>2) throw std::runtime_error("move() method takes 0-2 arguments");
	lua.pushValue(static_cast<lua_Integer>(rootWidget()->x()));
	lua.pushValue(static_cast<lua_Integer>(rootWidget()->y()));
	if(args.size()==2) rootWidget()->move(static_cast<int>(args[0].toInteger()),static_cast<int>(args[1].toInteger()));
	else if(args.size()==1) {
		if(args[0].toString()=="center") {
			auto geometry=QApplication::desktop()->screenGeometry(rootWidget());
			rootWidget()->move(geometry.center()-rootWidget()->rect().center());
		}
		else throw std::runtime_error(("Unrecognized argument: "+args[0].toString()).c_str());
	}
	return 2;
}

/*
 * Private members
 */

// For standalone windows, return this widget. For docked windows,
// return its parent QDockWidget

template <typename T> QWidget *LuaModelessDialog<T>::rootWidget() {
	auto dockWidget=dynamic_cast<QDockWidget*>(T::parent());
	if(dockWidget) return dockWidget;
	return this;
}

#endif
