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
 * This module implements members of the LuaDialogServer class.
 */

#include "luadialogserver.h"

#include "luaserver.h"
#include "fstring.h"
#include "luaformdialog.h"
#include "luatextviewer.h"
#include "luaplotterwidget.h"
#include "dockwrapper.h"
#include "mainwindow.h"
#include "filedialogex.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QApplication>
#include <QDesktopWidget>
#include <QSettings>

#include <vector>
#include <stdexcept>

using namespace std::placeholders;

extern MainWindow *g_MainWindow;

/*
 * LuaDialogServer members
 */

LuaGUIObject::Invoker LuaDialogServer::enumerateGUIMethods(int i,std::string &strName,InvokeType &type) {
	switch(i) {
	case 0:
		strName="screen";
		return std::bind(&LuaDialogServer::LuaMethod_screen,this,_1,_2);
	case 1:
		strName="messagebox";
		return std::bind(&LuaDialogServer::LuaMethod_messagebox,this,_1,_2);
	case 2:
		strName="inputdialog";
		return std::bind(&LuaDialogServer::LuaMethod_inputdialog,this,_1,_2);
	case 3:
		strName="filedialog";
		return std::bind(&LuaDialogServer::LuaMethod_filedialog,this,_1,_2);
	case 4:
		strName="createdialog";
		return std::bind(&LuaDialogServer::LuaMethod_createdialog,this,_1,_2);
	default:
		return Invoker();
	}
}

int LuaDialogServer::LuaMethod_screen(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(!args.empty()) throw std::runtime_error("screen() doesn't take arguments");
	auto geometry=QApplication::desktop()->screenGeometry();
	lua.pushValue(static_cast<lua_Integer>(geometry.width()));
	lua.pushValue(static_cast<lua_Integer>(geometry.height()));
	return 2;
}

int LuaDialogServer::LuaMethod_messagebox(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()<1||args.size()>4) throw std::runtime_error("messagebox() method takes 1-4 arguments");
	
	QMessageBox d(g_MainWindow);
	
	d.setText(FString(args[0].toString()));
	
	if(args.size()>=2) d.setWindowTitle(FString(args[1].toString()));
	else d.setWindowTitle(QObject::tr("Message"));
	
	if(args.size()>=3) {
		auto iconstr=args[2].toString();
		
		if(iconstr=="error") d.setIcon(QMessageBox::Critical);
		else if(iconstr=="warning") d.setIcon(QMessageBox::Warning);
		else if(iconstr=="information") d.setIcon(QMessageBox::Information);
		else if(iconstr=="question") d.setIcon(QMessageBox::Question);
		else if(iconstr=="") d.setIcon(QMessageBox::NoIcon);
		else throw std::runtime_error("Bad icon type: \""+iconstr+"\"");
	}
	
	if(args.size()==4) {
		QMessageBox::StandardButtons buttons;
		QMessageBox::StandardButton defButton;
		auto buttonstr=args[3].toString();
		std::size_t i;
		
		i=buttonstr.find("ok");
		if(i!=std::string::npos) buttons|=QMessageBox::Ok;
		if(i==0) defButton=QMessageBox::Ok;
		
		i=buttonstr.find("cancel");
		if(i!=std::string::npos) buttons|=QMessageBox::Cancel;
		if(i==0) defButton=QMessageBox::Cancel;
		
		i=buttonstr.find("yes");
		if(i!=std::string::npos) buttons|=QMessageBox::Yes;
		if(i==0) defButton=QMessageBox::Yes;
		
		i=buttonstr.find("no");
		if(i!=std::string::npos) buttons|=QMessageBox::No;
		if(i==0) defButton=QMessageBox::No;
		
		if(buttons!=QMessageBox::StandardButtons()) {
			d.setStandardButtons(buttons);
			d.setDefaultButton(defButton);
		}
	}
	
	QMessageBox::StandardButton r=static_cast<QMessageBox::StandardButton>(d.exec());
	
	switch(r) {
	case QMessageBox::Ok:
		lua.pushValue("ok");
		break;
	case QMessageBox::Cancel:
		lua.pushValue("cancel");
		break;
	case QMessageBox::Yes:
		lua.pushValue("yes");
		break;
	case QMessageBox::No:
		lua.pushValue("no");
		break;
	default:
		lua.pushValue(LuaValue());
		break;
	}
	
	return 1;
}

int LuaDialogServer::LuaMethod_inputdialog(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()<2||args.size()>3) throw std::runtime_error("inputdialog() method takes 2-3 arguments");
	
	QInputDialog d(g_MainWindow);
	d.setWindowFlags(d.windowFlags()&~Qt::WindowContextHelpButtonHint);
	
	d.setWindowTitle(FString(args[0].toString()));
	d.setLabelText(FString(args[1].toString()));
	
	if(args.size()==3) {
		if(args[2].type()==LuaValue::Table) {
			QStringList items;
			for(auto it=args[2].table().cbegin();it!=args[2].table().cend();it++) {
				items<<FString(it->second.toString());
			}
			d.setComboBoxItems(items);
		}
		else d.setTextValue(FString(args[2].toString()));
	}
	
	if(d.exec()) lua.pushValue(LuaValue(FString(d.textValue())));
	else lua.pushValue(LuaValue());
	
	return 1;
}

int LuaDialogServer::LuaMethod_filedialog(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()<1||args.size()>2) throw std::runtime_error("filedialog() method takes 1-2 arguments");
	
	QSettings s;
	s.beginGroup("LuaDialogServer");
	
	FileDialogEx d(g_MainWindow);
	auto dir=s.value("Directory");
	if(dir.isValid()) d.setDirectory(dir.toString());
	
	auto const &mode=args[0].toString();
	if(mode=="open") {
		d.setAcceptMode(QFileDialog::AcceptOpen);
		d.setFileMode(QFileDialog::ExistingFile);
	}
	else if(mode=="save") {
		d.setAcceptMode(QFileDialog::AcceptSave);
		d.setFileMode(QFileDialog::AnyFile);
	}
	else if(mode=="dir") {
		d.setAcceptMode(QFileDialog::AcceptOpen);
		d.setFileMode(QFileDialog::Directory);
		d.setOptions(QFileDialog::ShowDirsOnly);
	}
	else throw std::runtime_error("Bad dialog type");
	
	if(args.size()>1&&mode!="dir") d.setNameFilter(FString(args[1].toString()));
	
	if(d.exec()) {
		const FString &filename=d.fileName();
		if(!filename.empty()) lua.pushValue(filename);
		else lua.pushValue(LuaValue());
		s.setValue("Directory",d.directory().absolutePath());
	}
	else lua.pushValue(LuaValue());
	
	return 1;
}

int LuaDialogServer::LuaMethod_createdialog(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.empty()) throw std::runtime_error("createdialog() method requires at least 1 argument");
	
	auto dialogType=args[0].toString();
	std::vector<LuaValue> dialogArgs(args.begin()+1,args.end());
	LuaGUIObject *d;
	
	if(dialogType=="progress") d=new LuaProgressDialog(g_MainWindow,dialogArgs);
	else if(dialogType=="form") d=new LuaFormDialog(g_MainWindow,dialogArgs);
	else if(dialogType=="textviewer") d=new LuaTextViewer(g_MainWindow,dialogArgs);
	else if(dialogType=="plotter") {
		auto dd=new LuaPlotterWidget(g_MainWindow,dialogArgs);
		auto dock=new DockWrapper(dd,"Plotter","Plotter");
		dock->setAttribute(Qt::WA_DeleteOnClose);
		g_MainWindow->dock(dock);
		QObject::connect(dock,&DockWrapper::windowStateChanged,dd,&PlotterWidget::zoomFit);
		d=dd;
	}
	else throw std::runtime_error(("Unrecognized dialog type: "+dialogType).c_str());
	
// Both Lua server and parent window are responsible
// for the dialog object's lifecycle. If deleted by one of them,
// it will automatically detach itself from the other.
	lua.pushValue(lua.addManagedObject(d));
	
	return 1;
}

/*
 * LuaProgressDialog members
 */

LuaProgressDialog::LuaProgressDialog(QWidget *p,const std::vector<LuaValue> &args): LuaModelessDialog(p) {
	setMinimumDuration(0);
	
	if(args.size()>0) setLabelText(FString(args[0].toString()));
	else setLabelText(tr("Operation in progress..."));
	
	if(args.size()==2) setMaximum(static_cast<int>(args[1].toInteger()));
	else if(args.size()==3) {
		setMinimum(static_cast<int>(args[1].toInteger()));
		setMaximum(static_cast<int>(args[2].toInteger()));
	}
	
	QObject::connect(this,&LuaProgressDialog::canceled,this,&LuaProgressDialog::notifyCancel);
}

LuaProgressDialog::~LuaProgressDialog() {
/*
 * Note: althought the object will be unregistered in LuaCallbackObject::~LuaCallbackObject(),
 * we want to do it here to avoid the object being accessed from the worker thread
 * while the derived class is already destroyed, but the base class it not
 */
	detach();
}

void LuaProgressDialog::notifyCancel() {
	_canceled=true;
}

LuaGUIObject::Invoker LuaProgressDialog::enumerateGUIMethods(int i,std::string &strName,InvokeType &type) {
	switch(i) {
	case 0:
		strName="setvalue";
		type=Async;
		return std::bind(&LuaProgressDialog::LuaMethod_setvalue,this,_1,_2);
	case 1:
		strName="settext";
		type=Async;
		return std::bind(&LuaProgressDialog::LuaMethod_settext,this,_1,_2);
	case 2:
		strName="setrange";
		type=Async;
		return std::bind(&LuaProgressDialog::LuaMethod_setrange,this,_1,_2);
	case 3:
		strName="reset";
		type=Async;
		return std::bind(&LuaProgressDialog::LuaMethod_reset,this,_1,_2);
	default:
		return LuaModelessDialog::enumerateGUIMethods(i-4,strName,type);
	}
}

int LuaProgressDialog::LuaMethod_setvalue(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=1) throw std::runtime_error("setvalue() method takes 1 argument");
	setValue(static_cast<int>(args[0].toInteger()));
	return 0;
}

int LuaProgressDialog::LuaMethod_settext(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=1) throw std::runtime_error("settext() method takes 1 argument");
	setLabelText(FString(args[0].toString()));
	return 0;
}

int LuaProgressDialog::LuaMethod_setrange(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=2) throw std::runtime_error("setrange() method takes 2 arguments");
	setRange(static_cast<int>(args[0].toInteger()),static_cast<int>(args[1].toInteger()));
	return 0;
}

int LuaProgressDialog::LuaMethod_reset(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()>0) throw std::runtime_error("reset() method doesn't take arguments");
	_canceled=false;
	reset();
	return 0;
}

std::function<int(LuaServer&)> LuaProgressDialog::enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
	switch(i) {
	case 0:
		strName="canceled";
		return std::bind(&LuaProgressDialog::LuaMethod_canceled,this,_1);
	default:
		return LuaModelessDialog::enumerateLuaMethods(i-1,strName,upvalues);
	}
}

// Note: this method will be called directly, bypassing LuaGUIObject marshaling
// It must not touch any QProgressDialog members

int LuaProgressDialog::LuaMethod_canceled(LuaServer &lua) {
	if(lua.argc()!=0) throw std::runtime_error("canceled() method doesn't take arguments");
	lua.pushValue(_canceled.load());
	return 1;
}
