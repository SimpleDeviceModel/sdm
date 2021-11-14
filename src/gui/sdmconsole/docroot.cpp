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
 * This module implements members of the DocRoot class and other
 * related classes.
 */

#include "appwidelock.h"

#include "docroot.h"

#include "sidebar.h"
#include "registermapwidget.h"
#include "plotterwidget.h"

#include "luaserver.h"
#include "sdmconfig.h"

#include <QIcon>
#include <QSettings>
#include <QCoreApplication>
#include <QLocale>

using namespace std::placeholders;

/*
 * DocFactory members
 */

// Note: objects are constructed in the GUI thread

SDMPluginLua *DocFactory::makePlugin(LuaServer &lua) {
	return new DocPlugin(lua);
}

SDMDeviceLua *DocFactory::makeDevice(LuaServer &lua) {
	return new DocDevice(lua);
}

SDMChannelLua *DocFactory::makeChannel(LuaServer &lua) {
	return new DocChannel(lua);
}

SDMSourceLua *DocFactory::makeSource(LuaServer &lua) {
	return new DocSource(lua);
}

DocFactory DocFactory::global;

/*
 * DocItem members
 */

void DocItem::setStockIcon(int column,const QString &str) {
	QString path;
	
	if(str==QObject::tr("Opened")) path=":/icons/opened.svg";
	else if(str==QObject::tr("Closed")) path=":/icons/closed.svg";
	else if(str==QObject::tr("Connected")) path=":/icons/connected.svg";
	else if(str==QObject::tr("Not connected")) path=":/icons/notconnected.svg";
	
	if(!path.isEmpty()) {
		setIcon(column,QIcon(path));
		setToolTip(column,str);
	}
	else {
		setText(column,str);
	}
}

void DocItem::setTextAndTooltip(int column,const QString &str) {
	setText(column,str);
	setToolTip(column,str);
}

/*
 * DocRoot members
 */

DocRoot::DocRoot(LuaServer &l): LuaBridge(l) {
	addPluginSearchPath(Config::pluginsDir().str());
	
	setInfoTag("host",FString(QCoreApplication::applicationName()));
	
	LuaValue t;
	t.newarray();
	auto langs=QLocale().uiLanguages();
	for(auto const &lang: langs) t.array().push_back(FString(lang));
	setInfoTag("uilanguages",t);
}

DocRoot::~DocRoot() {
	if(!sideBar.isNull()) sideBar->treeWidget().clear();
}

BridgeFactory &DocRoot::factory() const {
	return DocFactory::global;
}

void DocRoot::setSideBar(SideBar *w) {
	sideBar=w;
}

DocPlugin &DocRoot::addPluginItem(const std::string &path) {
	return *marshal([&]{
		DocPlugin &plugin=LuaBridge::addPluginItem(path).cast<DocPlugin>();
		plugin.setCallbackMutex(callbackMutex());
		if(!sideBar.isNull()) {
			sideBar->treeWidget().addTopLevelItem(&plugin);
		}
		return &plugin;
	});
}

std::function<int(LuaServer&)> DocRoot::enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
	switch(i) {
	case 0:
		strName="selected";
		return std::bind(&DocRoot::LuaMethod_selected,this,_1);
	default:
		return LuaBridge::enumerateLuaMethods(i-1,strName,upvalues);
	}
}

int DocRoot::LuaMethod_selected(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("selected() member doesn't take arguments");
	
	LuaValue t;
	t.newtable();
	
	marshal([&]{
		auto item=sideBar->treeWidget().currentItem();
		
		if(auto source=dynamic_cast<DocSource*>(item)) {
			t.table()["source"]=source->luaHandle();
			item=source->QTreeWidgetItem::parent();
		}
		else if(auto channel=dynamic_cast<DocChannel*>(item)) {
			t.table()["channel"]=channel->luaHandle();
			item=channel->QTreeWidgetItem::parent();
		}
		
		if(auto device=dynamic_cast<DocDevice*>(item)) {
			t.table()["device"]=device->luaHandle();
			item=device->QTreeWidgetItem::parent();
		}
		
		if(auto plugin=dynamic_cast<DocPlugin*>(item)) {
			t.table()["plugin"]=plugin->luaHandle();
		}
	});
	
	lua.pushValue(t);
	return 1;
}

/*
 * DocPlugin members
 */

DocPlugin::DocPlugin(LuaServer &l): SDMPluginLua(l),_panel(*this) {
	setIcon(0,QIcon(":/icons/plugin.svg"));
}

BridgeFactory &DocPlugin::factory() const {
	return DocFactory::global;
}

DocDevice &DocPlugin::addDeviceItem(int iDev) {
	return *marshal([&]{
		DocDevice &device=SDMPluginLua::addDeviceItem(iDev).cast<DocDevice>();
		device.setCallbackMutex(callbackMutex());
		if(!device.treeWidget()) QTreeWidgetItem::addChild(&device);
	// Add auto-opened channels (if any)
		for(std::size_t i=0;i<device.children();i++) {
			if(auto channel=dynamic_cast<DocChannel*>(&device.child(i))) {
				device.QTreeWidgetItem::addChild(channel);
				channel->setCallbackMutex(callbackMutex());
			}
			else if(auto source=dynamic_cast<DocSource*>(&device.child(i))) {
				device.QTreeWidgetItem::addChild(source);
				source->setCallbackMutex(callbackMutex());
			}
		}
		return &device;
	});
}

void DocPlugin::open(const std::string &strFileName) {
	SDMPluginLua::open(strFileName);
	
	marshal([&]{
		setTextAndTooltip(0,FString(name()));
		setStockIcon(1,QObject::tr("Opened"));
		auto const &addons=listProperties("UserScripts");
		for(std::size_t i=0;i+1<addons.size();i+=2)
			_panel.addButton(FString(addons[i]),FString(addons[i+1]));
		_panel.refresh();
	});
}

void DocPlugin::close() {
	marshal([this]{
		enableUnsafeDestruction(true);
		SDMPluginLua::close();
	});
}

/*
 * DocDevice members
 */

DocDevice::DocDevice(LuaServer &l): SDMDeviceLua(l),_panel(*this) {
	setIcon(0,QIcon(":/icons/device.svg"));
	_timer.setInterval(1000);
	QObject::connect(&_timer,&QTimer::timeout,this,&DocDevice::checkConnection);
}

BridgeFactory &DocDevice::factory() const {
	return DocFactory::global;
}

DocChannel &DocDevice::addChannelItem(int iChannel) {
	return *marshal([&]{
		DocChannel &channel=SDMDeviceLua::addChannelItem(iChannel).cast<DocChannel>();
		channel.setCallbackMutex(callbackMutex());
		if(!channel.treeWidget()) {
			QTreeWidgetItem::addChild(&channel);
			QSettings s;
			bool autoOpen=s.value("MainWindow/AutoOpenToolWindows",true).toBool();
			if(autoOpen) {
				try {
					channel.registerMap();
				}
				catch(std::exception &) {}
			}
		}
		return &channel;
	});
}

DocSource &DocDevice::addSourceItem(int iSource) {
	return *marshal([&]{
		DocSource &source=SDMDeviceLua::addSourceItem(iSource).cast<DocSource>();
		source.setCallbackMutex(callbackMutex());
		if(!source.treeWidget()) {
			QTreeWidgetItem::addChild(&source);
			QSettings s;
			bool autoOpen=s.value("MainWindow/AutoOpenToolWindows",true).toBool();
			if(autoOpen) {
				try {
					source.addViewers();
				}
				catch(std::exception &) {}
			}
		}
		return &source;
	});
}

void DocDevice::open(const SDMPlugin &pl,int iDev) {
	SDMDeviceLua::open(pl,iDev);
	
	marshal([&]{
		setTextAndTooltip(0,FString(name()));
		setStockIcon(1,QObject::tr("Not connected"));
		auto const &addons=listProperties("UserScripts");
		for(std::size_t i=0;i+1<addons.size();i+=2)
			_panel.addButton(FString(addons[i]),FString(addons[i+1]));
		_panel.refresh();
	});
}

void DocDevice::close() {
	marshal([this]{
		enableUnsafeDestruction(true);
		SDMDeviceLua::close();
	});
}

void DocDevice::connect() {
	marshal([&]{
		SDMDeviceLua::connect();
		setStockIcon(1,QObject::tr("Connected"));
		_connected=true;
		_panel.refresh();
		_timer.start();
	});
}

void DocDevice::disconnect() {
	marshal([&]{
		SDMDeviceLua::disconnect();
		setStockIcon(1,QObject::tr("Not connected"));
		_connected=false;
		_panel.refresh();
		_timer.stop();
	});
}

void DocDevice::checkConnection() {
	AppWideLock::lock_t lock=AppWideLock::getTimedLock(20);
	if(!lock) return;
	if(!isConnected()) {
		setStockIcon(1,QObject::tr("Not connected"));
		_connected=false;
		_panel.refresh();
		_timer.stop();
	}
}

/*
 * DocChannel members
 */

DocChannel::DocChannel(LuaServer &l): SDMChannelLua(l),_panel(*this,l) {
	setIcon(0,QIcon(":/icons/channel.svg"));
}

BridgeFactory &DocChannel::factory() const {
	return DocFactory::global;
}

void DocChannel::open(const SDMDevice &d,int ch) {
	SDMChannelLua::open(d,ch);
	_registerMapFileName=FString(getProperty("RegisterMapFile",""));
	
	marshal([&]{
		setTextAndTooltip(0,FString(name()));
		setStockIcon(1,QObject::tr("Opened"));
		auto const &addons=listProperties("UserScripts");
		for(std::size_t i=0;i+1<addons.size();i+=2)
			_panel.addButton(FString(addons[i]),FString(addons[i+1]));
		_panel.refresh();
	});
}

void DocChannel::close() {
	marshal([this]{
		enableUnsafeDestruction(true);
		SDMChannelLua::close();
	});
}

RegisterMapWidget *DocChannel::registerMap() {
	return _panel.registerMap(_registerMapFileName);
}

std::function<int(LuaServer&)> DocChannel::enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
	switch(i) {
	case 0:
		strName="registermap";
		return std::bind(&DocChannel::LuaMethod_registermap,this,_1);
	default:
		return SDMChannelLua::enumerateLuaMethods(i-1,strName,upvalues);
	}
}

int DocChannel::LuaMethod_registermap(LuaServer &lua) {
	if(lua.argc()!=0) throw std::runtime_error("registermap() method doesn't take arguments");
	LuaValue h;
	marshal([&]{h=registerMap()->handle();});
	lua.pushValue(h);
	return 1;
}

/*
 * DocSource members
 */

DocSource::DocSource(LuaServer &l): SDMSourceLua(l),_panel(*this) {
	setIcon(0,QIcon(":/icons/source.svg"));
}

BridgeFactory &DocSource::factory() const {
	return DocFactory::global;
}

void DocSource::open(const SDMDevice &d,int ch) {
	SDMSourceLua::open(d,ch);
	
	marshal([&]{
		setTextAndTooltip(0,FString(name()));
		setStockIcon(1,QObject::tr("Opened"));
		auto const &addons=listProperties("UserScripts");
		for(std::size_t i=0;i+1<addons.size();i+=2)
			_panel.addButton(FString(addons[i]),FString(addons[i+1]));
		_panel.refresh();
		_streamNames=listProperties("Streams"); // try to get stream names
	});
}

void DocSource::close() {
	marshal([this]{
		enableUnsafeDestruction(true);
		SDMSourceLua::close();
	});
}

int DocSource::readStreamErrors() {
	auto r=SDMSourceLua::readStreamErrors();
	if(!_initErrors) _initErrors=true;
	else if(r!=_errors) emit streamErrorsChanged(r);
	_errors=r;
	return r;
}

void DocSource::addViewers() {
	std::vector<int> streams;
	
	auto list=listProperties("ShowStreams");
	if(!list.empty()) {
// If the "ShowStreams" property is defined, show streams specified there
		for(auto const &s: list) {
			streams.push_back(std::stoi(s));
		}
	}
	else {
// Otherwise, show all streams
		for(std::size_t i=0;i<_streamNames.size();i++)
			streams.push_back(static_cast<int>(i));
	}
	
	_panel.addViewer(streams,PlotterWidget::Preferred);
}

std::function<int(LuaServer&)> DocSource::enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
	switch(i) {
	case 0:
		strName="addviewer";
		return std::bind(&DocSource::LuaMethod_addviewer,this,_1);
	default:
		return SDMSourceLua::enumerateLuaMethods(i-1,strName,upvalues);
	}
}

int DocSource::LuaMethod_addviewer(LuaServer &lua) {
	if(lua.argc()==0||lua.argc()>3) throw std::runtime_error("addviewer() method takes 1-3 arguments");
	
	if(lua.argt(0)!=LuaValue::Table) throw std::runtime_error("First argument must be of table type");
	
	auto const &t=lua.argv(0);
	std::vector<int> streams;
	for(auto const &v: t.table()) streams.push_back(static_cast<int>(v.second.toInteger()));
	
	PlotterWidget::PlotMode mode=PlotterWidget::Preferred;
	if(lua.argc()>=2) {
		auto const &modeStr=lua.argv(1).toString();
		if(modeStr=="bars") mode=PlotterWidget::Bars;
		else if(modeStr=="plot") mode=PlotterWidget::Plot;
		else if(modeStr=="bitmap") mode=PlotterWidget::Bitmap;
		else if(modeStr=="video") mode=PlotterWidget::Video;
		else if(modeStr=="binary") mode=PlotterWidget::Binary;
		else throw std::runtime_error("Bad second argument: \"bars\", \"plot\", \"bitmap\", \"video\" or \"binary\" expected");
	}
	
	int multiLayer=0;
	if(lua.argc()>=3&&streams.size()==1) multiLayer=static_cast<int>(lua.argv(2).toInteger());
	
	marshal([&]{_panel.addViewer(streams,mode,multiLayer);});
	
	return 0;
}
