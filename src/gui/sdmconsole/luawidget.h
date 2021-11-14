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
 * This header file declares a LuaWidget class which implements
 * AbstractConsole interface by inheriting function-related
 * members from LuaConsole and represetation-related members
 * from ConsoleWidget.
 */

#ifndef LUAWIDGET_H_INCLUDED
#define LUAWIDGET_H_INCLUDED

#ifdef _MSC_VER
	#pragma warning(disable: 4250) // disable MSVC warnings about virtual base
#endif

#include "luaconsole.h"
#include "consolewidget.h"
#include "marshal.h"

#include <QTimer>

class QDragEnterEvent;
class QDropEvent;

class LuaWidget : public ConsoleWidget,public LuaConsole,public Marshal {
	Q_OBJECT
	
	LuaServer &lua;
	QTimer timer;
	FString lastPrompt;
	bool forcePrompt;
	bool dirty;
	
	static int promptDelay;
	
	void updatePrompt();
	
protected:
	virtual LuaServer::Completer createCompletionFunctor() override;
	
public:
	LuaWidget(LuaServer &l,const FString &name="",QWidget *parent=NULL);
	virtual ~LuaWidget();

	virtual void consoleCommand(const std::string &cmd) override;
	virtual void consolePrompt(const std::string &prompt) override;
	virtual void consoleOutput(const std::string &output) override;
	
public slots:
	void luaBusy(bool b);

protected:
	virtual void dragEnterEvent(QDragEnterEvent *e) override;
	virtual void dropEvent(QDropEvent *e) override;
};

#endif
