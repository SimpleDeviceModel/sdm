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
 * This is a main module for the SDMConsole program.
 */

#include "appwidelock.h"
#include "luaserverqt.h"
#include "fstring.h"
#include "sdmconfig.h"
#include "mainwindow.h"
#include "ioredirector.h"
#include "luatextcodec.h"
#include "luadialogserver.h"
#include "translations.h"
#include "cmdargs.h"
#include "luaiterator.h"
#include "u8eio.h"

#include <QApplication>
#include <QMessageBox>
#include <QIcon>
#include <QSettings>
#include <QLoggingCategory>

#include <string>
#include <exception>
#include <thread>
#include <cstdlib>
#include <clocale>

// Include header for _exit()

#ifdef _WIN32
	#include <stdlib.h>
#else
	#include <unistd.h>
#endif

IORedirector g_Redirector;
MainWindow *g_MainWindow;

static void uncleanExit(int msec) {
	std::this_thread::sleep_for(std::chrono::milliseconds(msec));
	_exit(0);
}

int main(int argc,char *argv[])
#ifdef NDEBUG
try
#endif
{
// Set up Qt prefix directory for Windows
#ifdef _WIN32
	QCoreApplication::setLibraryPaths({FString(Config::qtDir().str())});
#endif

	try {
		CmdArgs::global.parseCommandLine(argc,argv);
/* 
 * Under Microsoft Windows, IORedirector implements two methods of redirection.
 * The default method works by relaunching the current process with redirected
 * standard IO handles. IORedirector::start() returning IORedirector::RequestExit
 * means that the application should quit. The alternative method is less
 * reliable but doesn't relaunch the current process and will never return
 * IORedirector::RequestExit.
 *
 * Under Unix-like systems, IORedirector::start() will never return
 * IORedirector::RequestExit.
 */
		if(!CmdArgs::global.noRedirector) {
			auto r=g_Redirector.start(CmdArgs::global.altRedirector);
			if(r==IORedirector::RequestExit) return 0;
		}
	}
	catch(std::exception &ex) {
		QApplication app(argc,argv);
		QMessageBox::critical(NULL,QObject::tr("Fatal error"),ex.what(),QMessageBox::Ok);
		return EXIT_FAILURE;
	}
	
// Ensure that UTF-8 streams are synchronized after redirection
	u8e::utf8cin().sync();
	u8e::utf8cout().flush();
	u8e::utf8cerr().flush();
	
	QApplication app(argc,argv);
	
// Allow QIcon::pixmap() to return high-DPI pixmaps in device pixels
// as opposed to logical pixels
	QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
	
/* 
 * On POSIX platforms QApplication constructor calls setlocale(LC_ALL,"").
 * However, we want Lua to use decimal points regardless of the system locale.
 * For GUI, we use Qt facilities which don't depend on the Standard C library
 * locale.
 */
	std::setlocale(LC_NUMERIC,"C");
	
// Disable spam log messages
	QLoggingCategory::setFilterRules("*=false\n");
	
// Set application names
	QCoreApplication::setOrganizationName("SimpleDeviceModel");
	QCoreApplication::setApplicationName("sdmconsole");
	
// Initialize settings
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings s;
	if(s.value("Main/Reset").toBool()) s.clear();

// Deploy translations
	Translations::setLocale();

// Set application icon
	QIcon icon;
	icon.addFile(":/icons/appicon.svg");
	QApplication::setWindowIcon(icon);
	
// Instantiate miscellaneous objects
	LuaServerQt lua;
	
	DocRoot doc(lua);
	doc.setCallbackMutex(&AppWideLock::mutex());
	
// Configure Lua server
	auto it=lua.getGlobalIterator("package");
	
	auto itPath=it.valueIterator("path");
	auto newPath=Config::luaModulePath();
#ifdef USING_SYSTEM_LUA
	if(!newPath.empty()&&newPath.back()!=';') newPath.push_back(';');
	itPath.setValue(newPath+itPath->second.toString());
#else
	itPath.setValue(newPath);
#endif
	
	auto itCPath=it.valueIterator("cpath");
	auto newCPath=Config::luaCModulePath(LUA_VERSION_MAJOR LUA_VERSION_MINOR);
#ifdef USING_SYSTEM_LUA
	if(!newCPath.empty()&&newCPath.back()!=';') newCPath.push_back(';');
	itCPath.setValue(newCPath+itCPath->second.toString());
#else
	itCPath.setValue(newCPath);
#endif
	
	lua.setGlobal("sdm",doc.luaHandle());
	lua.setGlobal("codec",lua.addManagedObject(new LuaTextCodec));
	lua.setGlobal("gui",lua.addManagedObject(new LuaDialogServer));
	
	if(!lua.checkRuntime())
		std::cout<<FString(QObject::tr("Warning: main program and Lua interpreter "
			"use different instances of the Standard C library"))<<std::endl;
	
// This scope is intended to ensure MainWindow instance destruction
// when we are forced to resort to an unclean exit
	int r;
	{
		MainWindow w(lua,doc);
		g_MainWindow=&w;
		
		w.setWindowTitle(app.applicationName());
		w.show();
		
		r=app.exec();
	} // <== MainWindow instance will be destroyed here
	
// Set up defferred process termination (in the case when one of the objects
// can't be destroyed in reasonable time)	
	std::thread t(uncleanExit,500);
	t.detach();
	
	return r;
}
#ifdef NDEBUG
catch(std::exception &ex) {
	QApplication app(argc,argv);
	QMessageBox::critical(NULL,QObject::tr("Fatal error"),ex.what(),QMessageBox::Ok);
	return EXIT_FAILURE;
}
catch(...) {
	QApplication app(argc,argv);
	QMessageBox::critical(NULL,QObject::tr("Fatal error"),QObject::tr("Unexpected exception"),QMessageBox::Ok);
	return EXIT_FAILURE;
}
#endif
