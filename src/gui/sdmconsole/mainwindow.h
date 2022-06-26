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
 * This header file declares MainWindow class which represents
 * SDMConsole main window. It is derived from QMainWindow.
 */

#ifndef MAINWINDOW_H_INCLUDED
#define MAINWINDOW_H_INCLUDED

#include "docroot.h"
#include "dockwrapper.h"
#include "marshal.h"

#include <QApplication>
#include <QMainWindow>
#include <QPointer>
#include <QKeySequence>

#include <set>

class QSplitter;
class QLabel;

class QCloseEvent;
class QResizeEvent;

class QUrl;

class LuaServerQt;
class DockWrapper;
class SideBar;
class LuaWidget;
struct LuaCallResult;

class MainWindow : public QMainWindow,public Marshal {
	Q_OBJECT
	
	LuaServerQt &_lua;
	DocRoot &_docroot;

// Child widgets
	QSplitter *_splitter;
	SideBar *_sidebar;
	QMainWindow *_dockArea;
	LuaWidget *_luaConsole;
	DockWrapper *_luaDock;
	
// Status bar related members
	QLabel *_luaStatusWidget;
	
// Dock widget management
	std::set<QPointer<DockWrapper> > _dockWidgets;
	bool _groupDocks=true;

// Window settings saved before entering the fullscreen mode
	bool _wasMaximized;
	QRect _normalGeometry;
	
// Other
	int _sidebarSize;
	bool _fullScreenHint=true;
	QKeySequence _fullScreenShortcut;
	
public:
	MainWindow(LuaServerQt &l,DocRoot &d,QWidget *parent=NULL);
	
	virtual QSize sizeHint() const override;
	void dock(DockWrapper *w,Qt::DockWidgetArea where=Qt::RightDockWidgetArea,Qt::Orientation how=Qt::Vertical);

public slots:
	void menuViewFullScreen(bool b);
	void menuViewGroupDocks(bool b);
	void menuViewAutoOpenToolWindows(bool b);
	
	void menuLuaTerminateLuaScript();
	void menuLuaStats();
	void menuLuaRunScriptFromFile();
	
	void menuHelpManual();
	void menuHelpLuaHelp();
	void menuHelpAbout();
	
	void menuSettingsDisplayHints(bool b);
	void menuSettingsReset();
	
	void executeScript(const QString &path);
	void infoUrlHandler(const QUrl &url);
	
private slots:
	void luaStatusChanged(bool b);
	void arrangeDockWidgets();

signals:
	void dockChange();

protected:
	virtual void closeEvent(QCloseEvent *) override;
	virtual void resizeEvent(QResizeEvent *) override;

private:
	void constructMainMenu();
	void populateScriptsMenu(QMenu *menu,const QString &path);
	void populateLanguageMenu(QMenu *menu);
	void saveMetrics();
	void restoreMetrics();
	void fullScreenHint();
	
	void luaCompleter(const LuaCallResult &res);
};

#endif
