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
 * This module implements members of the MainWindow class.
 */

#include "appwidelock.h"

#include "mainwindow.h"

#include "luaserverqt.h"
#include "luawidget.h"
#include "sidebar.h"
#include "translations.h"
#include "fruntime_error.h"
#include "filedialogex.h"
#include "cmdargs.h"
#include "u8efile.h"
#include "csvparser.h"
#include "textviewer.h"

#include "sdmconfig.h"

#include <QApplication>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QSplitter>
#include <QLabel>
#include <QTextStream>
#include <QIcon>
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QFontDatabase>
#include <QStyle>

#include <map>
#include <functional>

MainWindow::MainWindow(LuaServerQt &l,DocRoot &d,QWidget *parent):
	QMainWindow(parent),
	_lua(l),
	_docroot(d)
{
// Create child widgets
	_sidebar=new SideBar(_docroot);
	_docroot.setSideBar(_sidebar);
	
	_dockArea=new QMainWindow;
	_dockArea->setDockOptions(QMainWindow::AnimatedDocks|QMainWindow::AllowNestedDocks|QMainWindow::AllowTabbedDocks);
	
	_splitter=new QSplitter(Qt::Horizontal);
	_splitter->addWidget(_sidebar);
	_splitter->addWidget(_dockArea);
	setCentralWidget(_splitter);
	
	_luaConsole=new LuaWidget(_lua,"console",this);
	_luaDock=new DockWrapper(_luaConsole,"LuaConsole",tr("Lua console"));
	dock(_luaDock);
	_luaConsole->setFocus();
	
	_luaStatusWidget=new QLabel;
	statusBar()->addWidget(_luaStatusWidget);
	luaStatusChanged(false);

// Connect "quit" signal from Lua console
	QObject::connect(_luaConsole,&ConsoleWidget::requestQuit,this,&MainWindow::close);
	
// Set up Lua notifications
	QObject::connect(&_lua,&LuaServerQt::statusChanged,this,&MainWindow::luaStatusChanged);
	QObject::connect(&_lua,&LuaServerQt::statusChanged,_luaConsole,&LuaWidget::luaBusy);

// Set up dock widget notifications
	QObject::connect(this,&MainWindow::dockChange,this,&MainWindow::arrangeDockWidgets,Qt::QueuedConnection);

// Set up geometry
	restoreMetrics();
	QObject::connect(_splitter,&QSplitter::splitterMoved,[this](int pos,int i){_sidebarSize=pos;});
	
// Obtain full screen keyboard shortcut
	_fullScreenShortcut=QKeySequence::FullScreen;
	if(_fullScreenShortcut.isEmpty()) _fullScreenShortcut=QKeySequence("F11");
	
// Create main menu
	constructMainMenu();
	
// Execute script from the command line
	if(!CmdArgs::global.scriptFile.empty()) {
// Push Lua arguments table
		LuaValue t;
		t.newtable();
		for(std::size_t i=0;i<CmdArgs::global.args.size();i++)
			t.table().emplace(static_cast<lua_Integer>(i-CmdArgs::global.scriptPos),
				CmdArgs::global.args[i]);
		_lua.setGlobal("arg",t);
		
		executeScript(FString(CmdArgs::global.scriptFile));
	}
	
	QDesktopServices::setUrlHandler("info",this,"infoUrlHandler");
}

QSize MainWindow::sizeHint() const {
	auto geometry=QApplication::desktop()->availableGeometry(this);
	auto w=geometry.width()*4/5;
	auto h=geometry.height()*4/5;
	return QSize(w,h);
}

void MainWindow::dock(DockWrapper *w,Qt::DockWidgetArea where,Qt::Orientation how) {
// Add new dock widgets
	_dockArea->addDockWidget(where,w,how);
	w->setDocked(true);
	QObject::connect(w,&QDockWidget::dockLocationChanged,this,&MainWindow::dockChange);
	QObject::connect(w,&QDockWidget::topLevelChanged,this,&MainWindow::dockChange);

// Clean up dock widgets set
	for(auto it=_dockWidgets.cbegin();it!=_dockWidgets.cend();) {
		auto newit=it;
		newit++;
		if(it->isNull()) _dockWidgets.erase(it);
		it=newit;
	}

// Tabify with similar if requested
	if(_groupDocks) {
		for(auto d: _dockWidgets) {
			if(w->type()==d->type()) {
				_dockArea->tabifyDockWidget(d,w);
				break;
			}
		}
	}

// Bring to the front
	w->show();
	w->raise();
	
// Add dock widget pointer to the set
	_dockWidgets.insert(w);
	
	emit dockChange();
}

/*
 * Main menu related member functions
 */

void MainWindow::constructMainMenu() {
	QSettings s;
	s.beginGroup("MainWindow");
	QMenu *m;
	QAction *a;
	
	m=menuBar()->addMenu(tr("&File"));
	m->addAction(QIcon(":/icons/plugin.svg"),tr("&Open plugin")+"...",
		_sidebar,SLOT(openPlugin()),QKeySequence::Open);
	m->addSeparator();
	m->addAction(QIcon(":/icons/quit.svg"),tr("&Exit"),
		this,SLOT(close()),QKeySequence::Quit);
	
	m=menuBar()->addMenu(tr("&View"));
	
	a=m->addAction(tr("Full screen mode"),this,
		SLOT(menuViewFullScreen(bool)),_fullScreenShortcut);
	a->setCheckable(true);
	a->setChecked(isFullScreen());
	m->addSeparator();
	
	m->addAction(_luaDock->toggleViewAction());
	m->addSeparator();
	
	a=m->addAction(tr("Group similar windows"),this,SLOT(menuViewGroupDocks(bool)));
	a->setCheckable(true);
	_groupDocks=s.value("GroupDocks",true).toBool();
	a->setChecked(_groupDocks);
	
	a=m->addAction(tr("Auto-open tool windows"),
		this,SLOT(menuViewAutoOpenToolWindows(bool)));
	a->setCheckable(true);
	auto autoOpenToolWindows=s.value("AutoOpenToolWindows",true).toBool();
	a->setChecked(autoOpenToolWindows);
	
	m=menuBar()->addMenu(tr("S&cripts"));
	populateScriptsMenu(m,FString(Config::scriptsDir().str()));
	m->addSeparator();
	a=m->addAction(QIcon(":/icons/run.svg"),tr("E&xecute")+"...",
		this,SLOT(menuLuaRunScriptFromFile()),QKeySequence("Ctrl+F5"));
	QObject::connect(&_lua,&LuaServerQt::statusChanged,a,&QAction::setDisabled);
	a=m->addAction(QIcon(":/icons/abort.svg"),tr("&Terminate"),
		this,SLOT(menuLuaTerminateLuaScript()),QKeySequence("Shift+F5"));
	a->setEnabled(false);
	QObject::connect(&_lua,&LuaServerQt::statusChanged,a,&QAction::setEnabled);
	m->addAction(QIcon(":/icons/lua-logo-nolabel.svg"),tr("Statistics"),
		this,SLOT(menuLuaStats()));
	
	m=menuBar()->addMenu(tr("&Settings"));
	auto langMenu=m->addMenu(QIcon(":/icons/language.svg"),tr("Language"));
	populateLanguageMenu(langMenu);
	m->addAction(QIcon(":/commonwidgets/icons/font.svg"),tr("Console font")+"...",
		_luaConsole,SLOT(menuChooseFont()));
	m->addAction(QIcon(":/icons/reset.svg"),tr("Reset to defaults")+"...",
		this,SLOT(menuSettingsReset()));
	
	m=menuBar()->addMenu(tr("&Help"));
	m->addAction(QIcon(":/icons/help.svg"),tr("User &manual"),
		this,SLOT(menuHelpManual()),QKeySequence::HelpContents);
	m->addAction(QIcon(":/icons/lua-logo-nolabel.svg"),tr("Lua &reference"),
		this,SLOT(menuHelpLuaHelp()),QKeySequence("Ctrl+L"));
	m->addSeparator();
	m->addAction(QIcon(":/icons/appicon.svg"),tr("About SDM"),
		this,SLOT(menuHelpAbout()));
}

void MainWindow::populateScriptsMenu(QMenu *menu,const QString &path) {
	QLocale locale;
	QDir dir(path);
	
// Load "*.lst" files (if any)
	std::map<QString,QString> names;
	auto namesFiles=dir.entryList(QStringList("*.lst"),QDir::Files);
	for(auto const &namesFile: namesFiles) {
		u8e::IFileStream in(FString(dir.absoluteFilePath(namesFile)).c_str());
		while(in) {
			auto record=CSVParser::getRecord(in);
			if(record.size()<2) continue;
// Third field, if present, specifies language
			if(record.size()>=3&&!locale.bcp47Name().startsWith(FString(record[2]))) continue;
			names[FString(record[0])]=FString(record[1]);
		}
	}
	
// Iterate over subdirectories (sorted by entry names)
	auto subdirs=dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
	std::map<QString,QString> subdirEntries;
	for(auto const &subdir: subdirs) {
		auto it=names.find(subdir);
		if(it!=names.end()) {
// Empty name string means that the subdirectory must not be included in the menu
			if(!it->second.isEmpty()) subdirEntries.emplace(it->second,subdir);
		}
		else subdirEntries.emplace(subdir,subdir);
	}
	for(auto const &entry: subdirEntries) {
		auto const &subdirPath=dir.absoluteFilePath(entry.second);
		auto submenu=menu->addMenu(entry.first);
		populateScriptsMenu(submenu,subdirPath);
	}
	
// Add scripts (sorted by entry names)
	auto scripts=dir.entryList(QStringList("*.lua"),QDir::Files);
	std::map<QString,QString> scriptEntries;
	for(auto script: scripts) {
		auto it=names.find(script);
		if(it!=names.end()) {
// Empty name string means that the script must not be included in the menu
			if(!it->second.isEmpty()) scriptEntries.emplace(it->second,script);
		}
		else {
			auto name=script;
			if(name.endsWith(".lua",Qt::CaseInsensitive)) name.chop(4);
			scriptEntries.emplace(name,script);
		}
	}
	for(auto const &entry: scriptEntries) {
		auto const &scriptPath=dir.absoluteFilePath(entry.second);
		auto action=menu->addAction(entry.first);
		QObject::connect(action,&QAction::triggered,
			std::bind(&MainWindow::executeScript,this,scriptPath));
	}
}

void MainWindow::populateLanguageMenu(QMenu *menu) {
	auto languages=Translations::supportedLanguages();
	auto langGroup=new QActionGroup(this);
	for(auto const &l: languages) {
		auto action=langGroup->addAction(l.second);
		menu->addAction(action);
		QObject::connect(action,&QAction::triggered,[=]{
			Translations::selectLanguage(l.first);
			QMessageBox::information(nullptr,QCoreApplication::applicationName(),tr("Please restart the program to apply settings"),QMessageBox::Ok);
		});
		action->setCheckable(true);
		if(l.first==Translations::currentLanguage()) action->setChecked(true);
		if(l.first<0) menu->addSeparator();
	}
}

/*
 * QT Slots
 */

void MainWindow::menuViewFullScreen(bool b) {
/*
 * Sadly, full screen window support is broken on multiple platforms.
 * In particular, on Microsoft Windows entering the fullscreen mode
 * loses information about normalized geometry. Workarounds follow.
 */
	if(b) {
		_wasMaximized=isMaximized();
/* 
 * Sometimes, when the window is maximized, normalized geometry
 * is not available, and normalGeometry() will return maximized
 * geometry. Don't save it in this case.
 */
		auto ng=normalGeometry();
		if(!isMaximized()||ng!=geometry()) _normalGeometry=ng;
		showFullScreen();
		fullScreenHint();
	}
	else {
		showNormal();
		if(_normalGeometry.isValid()) setGeometry(_normalGeometry);
		else adjustSize();
		if(_wasMaximized) showMaximized();
	}
}

void MainWindow::menuViewGroupDocks(bool b) {
	_groupDocks=b;
	QSettings s;
	s.setValue("MainWindow/GroupDocks",b);
}

void MainWindow::menuViewAutoOpenToolWindows(bool b) {
	QSettings s;
	s.setValue("MainWindow/AutoOpenToolWindows",b);
}

void MainWindow::menuLuaTerminateLuaScript() {
	if(!_lua.busy()) {
		QMessageBox::critical(this,tr("Error"),tr("Lua interpreter is not currently running"),QMessageBox::Ok);
		return;
	}
	_lua.terminate();
}

void MainWindow::menuLuaStats() try {
	QMessageBox msgBox(this);
	
	QString str;
	QTextStream ts(&str);
	ts<<"<table width=\"100%\">\n";
	ts<<"<tr><td>"<<tr("Interpreter version")<<"</td><td>"<<QString(LUA_RELEASE).replace("Lua ","")<<"</td></tr>\n"<<endl;
	ts<<"<tr><td>"<<tr("Number of calls")<<"</td><td>"<<_lua.calls()<<"</td></tr>\n"<<endl;
	ts<<"<tr><td>"<<tr("Total execution time")<<"</td><td>"<<_lua.msecTotal()<<"&nbsp;"<<tr("ms")<<"</td></tr>\n"<<endl;
	
	try {
		auto kb=_lua.kbRam();
		ts<<"<tr><td>"<<tr("Memory used")<<"</td><td>"<<kb<<"&nbsp;"<<tr("kB")<<"</td></tr>\n";
		ts<<"<tr><td>"<<tr("Status")<<"</td><td>"<<tr("Ready")<<"</td></tr>\n";
	}
	catch(std::exception &) {
		ts<<"<tr><td>"<<tr("Status")<<"</td><td>"<<tr("Busy")<<"</td></tr>\n";
	}
	
	ts<<"</table>";
	
	msgBox.setWindowTitle(QCoreApplication::applicationName());
	msgBox.setTextFormat(Qt::RichText);
	msgBox.setText("<h3>"+tr("Lua statistics")+"</h3>");
	msgBox.setInformativeText(str);
	auto iconWidth=QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize)*2;
	msgBox.setIconPixmap(QIcon(":/icons/lua-logo-nolabel.svg").pixmap(iconWidth));
	msgBox.exec();
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void MainWindow::menuLuaRunScriptFromFile() try {
	if(_lua.busy()) throw fruntime_error(tr("Lua interpreter is busy"));
	
	QSettings s;
	s.beginGroup("MainWindow");
	
	FileDialogEx d;
	d.setWindowTitle(tr("Execute script"));
	d.setAcceptMode(QFileDialog::AcceptOpen);
	d.setFileMode(QFileDialog::ExistingFile);
	d.setNameFilter(tr("Lua scripts (*.lua);;All files (*)"));
	
	auto dir=s.value("ScriptDirectory");
	if(dir.isValid()) d.setDirectory(dir.toString());
	
	int r=d.exec();
	if(!r) return;
	
	s.setValue("ScriptDirectory",d.directory().absolutePath());
	
	executeScript(d.fileName());
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void MainWindow::menuSettingsReset() {
	auto r=QMessageBox::question(this,QCoreApplication::applicationName(),
		tr("Reset program settings to defaults?"),
		QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes);
	if(r!=QMessageBox::Yes) return;
	
// Schedule settings reset during the next program startup
	QSettings s;
	s.setValue("Main/Reset",true);
	
	QMessageBox::information(nullptr,QCoreApplication::applicationName(),tr("Please restart the program to apply settings"),QMessageBox::Ok);
}

void MainWindow::menuHelpManual() {
	Path url=Config::docDir()+"manual.pdf";
	QDesktopServices::openUrl(QUrl::fromLocalFile(FString(url.str())));
}

void MainWindow::menuHelpLuaHelp() {
	Path url=Config::docDir()+"lua/contents.html";
	QDesktopServices::openUrl(QUrl::fromLocalFile(FString(url.str())));
}

void MainWindow::menuHelpAbout() {
	QMessageBox msgBox(this);
	msgBox.setWindowTitle(tr("About SDM"));
	msgBox.setTextFormat(Qt::RichText);
	msgBox.setText(QStringLiteral("<h3>Simple Device Model %1 (%2)</h3>").arg(Config::version(),Config::architecture()));
	
	auto text=tr("<p>Simple Device Model is an open source framework for instrument control and data acquisition.</p>\n")+
		tr("<p><a href=\"info://build\">Build information</a></p>\n")+
		tr("<p><a href=\"info://doc/changelog.txt\">Changelog</a></p>\n")+
		tr("<p><a href=\"http://www.micro-project.ru\">Website</a></p>\n")+
		tr("<h4>Legal information</h4>\n")+
		tr("<p>This program comes with ABSOLUTELY NO WARRANTY. This is free software, and you are welcome to redistribute it under certain conditions. See the link below for details.</p>\n")+
		tr("<p><a href=\"info://doc/licenses/license.txt\">Copyright and licensing information</a></p>\n");
	
	msgBox.setInformativeText(text);
	auto iconWidth=QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize)*2;
	msgBox.setIconPixmap(QApplication::windowIcon().pixmap(iconWidth));
	msgBox.exec();
}

void MainWindow::executeScript(const QString &path) try {
	if(_lua.busy()) throw fruntime_error(tr("Lua interpreter is busy"));
	const FString filename=Path(FString(path)).toAbsolute().str();
	u8e::IFileStream in(filename.c_str(),std::ios_base::in|std::ios_base::binary);
	if(!in) throw fruntime_error(tr("Cannot open script file \"")+filename+"\"");
	LuaStreamReader reader(in,true);
	_lua.executeChunkAsync(reader,"@"+filename,
		prepareMarshaledFunctor<const LuaCallResult&>
			(std::bind(&MainWindow::luaCompleter,this,std::placeholders::_1)));
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void MainWindow::infoUrlHandler(const QUrl &url) try {
	auto urlHost=url.host();
	auto urlPath=url.path();
	if(urlHost.isEmpty()) return;
	
	TextViewer tv(this);
	
	if(urlHost=="doc") {
		if(urlPath.isEmpty()) return;
		if(urlPath[0]=='/') urlPath.remove(0,1);
		auto docPath=Config::docDir()+FString(urlPath);
		tv.loadFile(FString(docPath.str()));
		tv.exec();
	}
	else if(urlHost=="build") {
		tv.setWindowTitle(tr("Build information"));
		QString str;
		QTextStream ts(&str);
		ts<<tr("Version")<<": "<<Config::version()<<endl;
		ts<<tr("Platform")<<": "<<Config::os()<<" ("<<Config::architecture()<<")"<<endl;
		ts<<tr("Compiler")<<": "<<Config::compiler()<<endl;
		ts<<tr("Toolkit version")<<": "<<QT_VERSION_STR<<endl;
		if(*Config::commitHash()) ts<<tr("Commit hash")<<": "<<Config::commitHash()<<endl;
		if(Config::commitTimestamp()) ts<<tr("Commit timestamp")<<": "<<
			QLocale().toString(QDateTime::fromTime_t(Config::commitTimestamp()))<<endl;
		tv.loadString(str);
		tv.exec();
	}
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}


void MainWindow::arrangeDockWidgets() {
// This workaround is intended to force dock area layout recalculation
	_dockArea->setUpdatesEnabled(false);
	const QSize s=_dockArea->size();
	_dockArea->resize(_dockArea->minimumSize());
	_dockArea->resize(s);
	_dockArea->setUpdatesEnabled(true);
}

void MainWindow::saveMetrics() {
	QSettings s;
	s.beginGroup("MainWindow");
	s.setValue("Geometry",saveGeometry());
	s.setValue("SidebarWidth",_sidebarSize);
	if(isFullScreen()) {
		s.setValue("NormalGeometry",_normalGeometry);
		s.setValue("WasMaximized",_wasMaximized);
	}
}

void MainWindow::restoreMetrics() {
	QSettings s;
	s.beginGroup("MainWindow");
	auto geometry=s.value("Geometry");
	if(geometry.isValid()) restoreGeometry(geometry.toByteArray());
	else setWindowState(windowState()|Qt::WindowMaximized);
	_sidebarSize=s.value("SidebarWidth",-1).toInt();
	_wasMaximized=s.value("WasMaximized",false).toBool();
	_normalGeometry=s.value("NormalGeometry").toRect();
	if(windowState()&Qt::WindowFullScreen) fullScreenHint();
}

void MainWindow::fullScreenHint() {
	if(_fullScreenHint) {
		auto str=tr("Press %1 to exit full screen mode");
		str=str.arg(_fullScreenShortcut.toString(QKeySequence::NativeText));
		_luaConsole->consoleOutput(FString(str));
		_fullScreenHint=false;
	}
}

/*
 * Lua management
 */

void MainWindow::luaStatusChanged(bool b) {
	if(b) _luaStatusWidget->setText(tr("Lua status: Busy"));
	else _luaStatusWidget->setText(tr("Lua status: Ready"));
}

void MainWindow::luaCompleter(const LuaCallResult &res) {
	if(!res.success) {
		_luaConsole->consoleOutput(res.errorMessage);
		CmdArgs::global.batchMode=false;
	}
	
	if(CmdArgs::global.batchMode) close();
}

/*
 * Events
 */

void MainWindow::closeEvent(QCloseEvent *) {
	saveMetrics();
	QCoreApplication::quit();
}

void MainWindow::resizeEvent(QResizeEvent *e) {
	if(_sidebarSize<=0) _sidebarSize=QApplication::desktop()->availableGeometry(this).width()/5;
	_splitter->setSizes({_sidebarSize,width()-_sidebarSize});
	QMainWindow::resizeEvent(e);
}
