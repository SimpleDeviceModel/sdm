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
 * This header file defines a main class for register map widget
 * which allows the user to read and write SDM device registers.
 */

#ifndef REGISTERMAPWIDGET_H_INCLUDED
#define REGISTERMAPWIDGET_H_INCLUDED

#include "pointerwatcher.h"
#include "registermapengine.h"

#include <QMainWindow>
#include <QToolBar>
#include <QMenu>
#include <QLabel>

class DocChannel;
class LuaServer;
class RegisterMapWorker;

class RegisterMapWidget : public QMainWindow,public Marshal {
	Q_OBJECT
	
	LuaServer &lua;
	PointerWatcher<DocChannel> channel;
	
	QToolBar toolBar;
	RegisterMapEngine tabs;
	QLabel statusLabel;
	
	RegisterMap::NumberMode numMode;
	bool _autoWrite;
	
	bool _busy=false;
	
	PointerWatcher<RegisterMapWorker> _worker;
	
// Actions
	QAction *addRowAction;
	QAction *removeRowAction;
	QAction *upAction;
	QAction *downAction;
	QAction *writeAction;
	QAction *readAction;
	QAction *detailsAction;
	QAction *saveAction;
	QAction *loadAction;
	
public:
	RegisterMapWidget(LuaServer &l,DocChannel &ch);
	virtual ~RegisterMapWidget();
	
	const LuaValue &handle() const {return tabs.handle();}

public slots:
	void addPage();
	void removePage(int index);
	void editPageName(int index);

	void addRegister();
	void addFifo();
	void addMemory();
	void addSection();
	void removeRow();
	
	void rowUp();
	void rowDown();
	
	void writeReg();
	void writeRegByIndex(int page,int row);
	void writePage();
	void writeAll();
	void setAutoWrite(bool b);
	void readReg();
	void readRegByIndex(int page,int row);
	void readPage();
	void readAll();
	
	void configureRegister(int page=-1,int row=-1);
	
	void saveMap();
	void loadMap();
	void loadMap(const QString &filename);
	
	void updateToolbar();
	void setBusy();
	void setReady();
	void rowDataChangedSlot(int page,int row);

private:
	void rwReg(int page,int row,bool write);
	void rwPage(int page,bool write);
	void rwAll(bool write);
};

#endif
