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
 * This header file defines classes derived from LuaBridge library
 * to use in the sdmconsole application.
 */

#ifndef DOCROOT_H_INCLUDED
#define DOCROOT_H_INCLUDED

#include "luabridge.h"
#include "pointerwatcher.h"
#include "marshal.h"

#include "docpanels.h"

#include <QPointer>
#include <QTreeWidgetItem>
#include <QTimer>

class SideBar;
class PlotterWidget;

class DocFactory : public BridgeFactory,public Marshal {
public:
	virtual SDMPluginLua *makePlugin(LuaServer &lua) override;
	virtual SDMDeviceLua *makeDevice(LuaServer &lua) override;
	virtual SDMChannelLua *makeChannel(LuaServer &lua) override;
	virtual SDMSourceLua *makeSource(LuaServer &lua) override;
	
	static DocFactory global;
};

class DocItem : public QObject,public QTreeWidgetItem,public Marshal {
public:
	virtual QWidget* panel()=0;
protected:
	void setStockIcon(int column,const QString &str);
	void setTextAndTooltip(int column,const QString &str);
};

class DocChannel : public DocItem,public SDMChannelLua {
	LuaValue registerMapHandle;
	ChannelPanel _panel;
	QString _registerMapFileName;
public:
	DocChannel(LuaServer &l);
	
protected:
	virtual BridgeFactory &factory() const override;
	
	virtual std::function<int(LuaServer&)> enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) override;
	int LuaMethod_registermap(LuaServer &lua);

public:
	virtual void open(const SDMDevice &d,int ch) override;
	virtual void close() override;
	
	RegisterMapWidget *registerMap();
	
	using SDMChannelLua::parent;
	using SDMChannelLua::child;
	using SDMChannelLua::children;
	using SDMChannelLua::setProperty;
	
	virtual ChannelPanel *panel() override {return &_panel;}
};

class DocSource : public DocItem,public SDMSourceLua {
	Q_OBJECT
	
	SourcePanel _panel;
	std::vector<std::string> _streamNames;
	int _errors;
	bool _initErrors=false;
public:
	DocSource(LuaServer &l);
	
protected:
	virtual BridgeFactory &factory() const override;
	
	virtual std::function<int(LuaServer&)> enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) override;
	int LuaMethod_addviewer(LuaServer &lua);

public:
	virtual void open(const SDMDevice &d,int src) override;
	virtual void close() override;
	
	virtual int readStreamErrors() override;
	
	void addViewers();
	
	using SDMSourceLua::parent;
	using SDMSourceLua::child;
	using SDMSourceLua::children;
	using SDMSourceLua::setProperty;
	
	virtual SourcePanel *panel() override {return &_panel;}
	
	const std::vector<std::string> &streamNames() const {return _streamNames;}

signals:
	void streamErrorsChanged(int);
};

class DocDevice : public DocItem,public SDMDeviceLua {
	Q_OBJECT
	
	DevicePanel _panel;
	QTimer _timer;
	bool _connected=false;
public:
	DocDevice(LuaServer &l);

protected:
	virtual BridgeFactory &factory() const override;

public:
	bool connected() const {return _connected;}
	
	virtual DocChannel &addChannelItem(int iChannel) override;
	virtual DocSource &addSourceItem(int iSource) override;
	
	virtual void open(const SDMPlugin &pl,int iDev) override;
	virtual void close() override;
	virtual void connect() override;
	virtual void disconnect() override;
	
	using SDMDeviceLua::parent;
	using SDMDeviceLua::child;
	using SDMDeviceLua::children;
	using SDMDeviceLua::setProperty;
	
	virtual DevicePanel *panel() override {return &_panel;}

private slots:
	void checkConnection();
};

class DocPlugin : public DocItem,public SDMPluginLua {
	PluginPanel _panel;
public:
	DocPlugin(LuaServer &l);
protected:
	virtual BridgeFactory &factory() const override;

public:
	virtual DocDevice &addDeviceItem(int iDev)  override;
	
	virtual void open(const std::string &strFileName) override;
	virtual void close() override;
	
	using SDMPluginLua::parent;
	using SDMPluginLua::child;
	using SDMPluginLua::children;
	using SDMPluginLua::setProperty;
	
	virtual PluginPanel *panel() override {return &_panel;}
};

class DocRoot : public LuaBridge,public Marshal {
	QPointer<SideBar> sideBar;
public:
	DocRoot(LuaServer &l);
	virtual ~DocRoot();
	
	void setSideBar(SideBar *w);

// Override LuaBridge members
protected:
	virtual BridgeFactory &factory() const override;
	
	virtual std::function<int(LuaServer&)> enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) override;
	int LuaMethod_selected(LuaServer &lua);

public:
	virtual DocPlugin &addPluginItem(const std::string &path) override;
};

#endif
