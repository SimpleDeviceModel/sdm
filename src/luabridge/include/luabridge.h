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
 * This header file defines a set of "bridge" classes which make
 * SDMPlug library classes accessible from Lua.
 */

#ifndef LUABRIDGE_H_INCLUDED
#define LUABRIDGE_H_INCLUDED

#ifdef _MSC_VER
// disable useless MSVC warning about virtual inheritance
	 #pragma warning(disable: 4250)
#endif

#include "treeitem.h"
#include "luacallbackobject.h"
#include "sdmplug.h"

#include <memory>

class SDMPluginLua;
class SDMDeviceLua;
class SDMChannelLua;
class SDMSourceLua;

class BridgePropertyManager : virtual public SDMBase {
protected:
// Lua wrapper methods
	std::function<int(LuaServer&)> enumeratePropertyMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues);
	
	int LuaMethod_listproperties(LuaServer &lua);
	int LuaMethod_getproperty(LuaServer &lua);
	int LuaMethod_setproperty(LuaServer &lua);
	int LuaMethod_meta_index(LuaServer &lua);
	int LuaMethod_meta_newindex(LuaServer &lua);
};

class BridgeFactory {
public:
	virtual SDMPluginLua *makePlugin(LuaServer &lua);
	virtual SDMDeviceLua *makeDevice(LuaServer &lua);
	virtual SDMChannelLua *makeChannel(LuaServer &lua);
	virtual SDMSourceLua *makeSource(LuaServer &lua);
	
	static BridgeFactory global;
};

class LuaBridge : public LuaCallbackObject,public TreeItem {
	LuaServer &_lua;
	LuaValue _handle;
	std::vector<std::string> _pluginSearchPath;
	std::map<std::string,LuaValue> _infoTags;
 	std::shared_ptr<int> _lockCnt;

public:
	LuaBridge(LuaServer &l);
	virtual ~LuaBridge();
	
	virtual std::string objectType() const override {return "Bridge";}
	const LuaValue &luaHandle();
	
	void addPluginSearchPath(const std::string &str) {_pluginSearchPath.push_back(str);}
	virtual SDMPluginLua &addPluginItem(const std::string &strFileName);
	
	LuaValue infoTag(const std::string &name) const;
	void setInfoTag(const std::string &name,const LuaValue &value);
	
protected:
	virtual BridgeFactory &factory() const;

// Implement Lua interface
	virtual std::function<int(LuaServer&)> enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) override;
	int LuaMethod_openplugin(LuaServer &lua);
	int LuaMethod_plugins(LuaServer &lua);
	int LuaMethod_info(LuaServer &lua);
	int LuaMethod_path(LuaServer &lua);
	int LuaMethod_lock(LuaServer &lua);
	
	static int LuaMethod_sleep(LuaServer &lua);
	static int LuaMethod_time(LuaServer &lua);
private:
	static void lockFinalizer(callback_mutex_t *m,const std::shared_ptr<int> &cnt);
};

class SDMPluginLua : public TreeItem,public SDMPlugin,public LuaCallbackObject,private BridgePropertyManager {
	LuaServer &_lua;
	LuaValue _handle;

public:
	SDMPluginLua(LuaServer &l);
	virtual ~SDMPluginLua();

protected:
	virtual BridgeFactory &factory() const;

public:
	virtual std::string objectType() const override {return "Plugin";}
	const LuaValue &luaHandle();
	
	virtual void open(const std::string &path) override;
	virtual void close() override;
	
	virtual SDMDeviceLua &addDeviceItem(int iDev);

protected:
// Implement Lua interface
	virtual std::function<int(LuaServer&)> enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) override;
	int LuaMethod_path(LuaServer &lua);
	int LuaMethod_close(LuaServer &lua);
	int LuaMethod_opendevice(LuaServer &lua);
	int LuaMethod_devices(LuaServer &lua);
};

class SDMDeviceLua : public TreeItem,public SDMDevice,public LuaCallbackObject,private BridgePropertyManager {
	LuaServer &_lua;
	LuaValue _handle;

public:
	SDMDeviceLua(LuaServer &l);
	virtual ~SDMDeviceLua();
	
protected:
	virtual BridgeFactory &factory() const;

public:
	virtual std::string objectType() const override {return "Device";}
	const LuaValue &luaHandle();
	
	virtual void open(const SDMPlugin &pl,int iDev) override;
	virtual void close() override;
	
	virtual void connect() override;
	
	virtual SDMChannelLua &addChannelItem(int iChannel);
	virtual SDMSourceLua &addSourceItem(int iSource);

protected:
// Implement Lua interface
	virtual std::function<int(LuaServer&)> enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) override;
	int LuaMethod_id(LuaServer &lua);
	int LuaMethod_close(LuaServer &lua);
	int LuaMethod_connect(LuaServer &lua);
	int LuaMethod_disconnect(LuaServer &lua);
	int LuaMethod_isconnected(LuaServer &lua);
	int LuaMethod_openchannel(LuaServer &lua);
	int LuaMethod_channels(LuaServer &lua);
	int LuaMethod_opensource(LuaServer &lua);
	int LuaMethod_sources(LuaServer &lua);
};

class SDMChannelLua : public TreeItem,public SDMChannel,public LuaCallbackObject,private BridgePropertyManager {
	LuaServer &_lua;
	LuaValue _handle;

protected:
	virtual BridgeFactory &factory() const;

public:
	SDMChannelLua(LuaServer &l);
	virtual ~SDMChannelLua();
	
	virtual std::string objectType() const override {return "Channel";}
	const LuaValue &luaHandle();
	
	virtual void open(const SDMDevice &d,int ch) override;
	virtual void close() override;

protected:
// Implement Lua interface
	virtual std::function<int(LuaServer&)> enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) override;
	int LuaMethod_id(LuaServer &lua);
	int LuaMethod_close(LuaServer &lua);
	
	int LuaMethod_writereg(LuaServer &lua);
	int LuaMethod_readreg(LuaServer &lua);
	int LuaMethod_writefifo(LuaServer &lua);
	int LuaMethod_readfifo(LuaServer &lua);
	int LuaMethod_writemem(LuaServer &lua);
	int LuaMethod_readmem(LuaServer &lua);
};

class SDMSourceLua : public TreeItem,public SDMSource,public LuaCallbackObject,private BridgePropertyManager {
	LuaServer &_lua;
	LuaValue _handle;

protected:
	virtual BridgeFactory &factory() const;

public:
	SDMSourceLua(LuaServer &l);
	virtual ~SDMSourceLua();
	
	virtual std::string objectType() const override {return "Source";}
	const LuaValue &luaHandle();
	
	virtual void open(const SDMDevice &d,int ch) override;
	virtual void close() override;

protected:
// Implement Lua interface
	virtual std::function<int(LuaServer&)> enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) override;
	int LuaMethod_id(LuaServer &lua);
	int LuaMethod_close(LuaServer &lua);
	
	int LuaMethod_selectreadstreams(LuaServer &lua);
	int LuaMethod_readstream(LuaServer &lua);
	int LuaMethod_readnextpacket(LuaServer &lua);
	int LuaMethod_discardpackets(LuaServer &lua);
	int LuaMethod_readstreamerrors(LuaServer &lua);
};

#endif
