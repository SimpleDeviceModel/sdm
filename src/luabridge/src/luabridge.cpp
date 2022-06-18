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
 * This module implements members of LuaBridge library classes.
 */

#include "luabridge.h"
#include "luaserver.h"

#include "sdmconfig.h"

#include <thread>
#include <chrono>

using namespace std::placeholders;

/*
 * BridgePropertyManager members
 */

std::function<int(LuaServer&)> BridgePropertyManager::enumeratePropertyMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
	switch(i) {
	case 0:
		strName="listproperties";
		return std::bind(&BridgePropertyManager::LuaMethod_listproperties,this,_1);
	case 1:
		strName="getproperty";
		return std::bind(&BridgePropertyManager::LuaMethod_getproperty,this,_1);
	case 2:
		strName="setproperty";
		return std::bind(&BridgePropertyManager::LuaMethod_setproperty,this,_1);
	case 3:
		strName="__index";
		return std::bind(&BridgePropertyManager::LuaMethod_meta_index,this,_1);
	case 4:
		strName="__newindex";
		return std::bind(&BridgePropertyManager::LuaMethod_meta_newindex,this,_1);
	default:
		return std::function<int(LuaServer&)>();
	}
}

int BridgePropertyManager::LuaMethod_listproperties(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("listproperties() method takes 1 argument");
	
	const auto &list=listProperties(lua.argv(0).toString());
	
	LuaValue t;
	t.newtable();
	
	for(std::size_t i=0;i<list.size();i++) {
		t.table()[lua_Integer(i+1)]=list[i];
	}

	lua.pushValue(t);
	
	return 1;
}

int BridgePropertyManager::LuaMethod_getproperty(LuaServer &lua) {
	if(lua.argc()!=1&&lua.argc()!=2) throw std::runtime_error("getproperty() method takes 1-2 arguments");
	try {
		lua.pushValue(getProperty(lua.argv(0).toString()));
	}
	catch(std::exception &) { // no such property exists
		if(lua.argc()==1) lua.pushValue(LuaValue()); // return nil
		else lua.pushValue(lua.argv(1).toString()); // return default value
	}
	
	return 1;
}

int BridgePropertyManager::LuaMethod_setproperty(LuaServer &lua) {
	if(lua.argc()!=2) throw std::runtime_error("setproperty() method takes 2 arguments");
	setProperty(lua.argv(0).toString(),lua.argv(1).toString());
	return 0;
}

int BridgePropertyManager::LuaMethod_meta_index(LuaServer &lua) {
	if(lua.argc()!=2) return 0;
	try {
		lua.pushValue(getProperty(lua.argv(1).toString()));
	}
	catch(std::exception &) {
		lua.pushValue(LuaValue());
	}
	return 1;
}

int BridgePropertyManager::LuaMethod_meta_newindex(LuaServer &lua) {
	if(lua.argc()!=3) return 0;
	setProperty(lua.argv(1).toString(),lua.argv(2).toString());
	return 0;
}

/*
 * BridgeFactory members
 */

SDMPluginLua *BridgeFactory::makePlugin(LuaServer &lua) {
	return new SDMPluginLua(lua);
}

SDMDeviceLua *BridgeFactory::makeDevice(LuaServer &lua) {
	return new SDMDeviceLua(lua);
}

SDMChannelLua *BridgeFactory::makeChannel(LuaServer &lua) {
	return new SDMChannelLua(lua);
}

SDMSourceLua *BridgeFactory::makeSource(LuaServer &lua) {
	return new SDMSourceLua(lua);
}

BridgeFactory BridgeFactory::global;

/*
 * LuaBridge members
 */

LuaBridge::LuaBridge(LuaServer &l):
	_lua(l),
	_lockCnt(std::make_shared<int>(0))
{
	setInfoTag("host","undefined");
	setInfoTag("version",Config::version());
#ifdef _WIN32
	setInfoTag("os","windows");
#else
	setInfoTag("os","unix");
#endif
	LuaValue t;
	t.newarray();
	t.array().push_back("en-US");
	setInfoTag("uilanguages",t);
}

LuaBridge::~LuaBridge() {
	_lua.unregisterObject(*this);
}

BridgeFactory &LuaBridge::factory() const {
	return BridgeFactory::global;
}

const LuaValue &LuaBridge::luaHandle() {
	if(_handle.type()==LuaValue::Nil) {
		_handle=_lua.registerObject(*this);
		_handle.table()["sleep"]=_lua.registerCallback(LuaMethod_sleep);
		_handle.table()["time"]=_lua.registerCallback(LuaMethod_time);
	}
	return _handle;
}

SDMPluginLua &LuaBridge::addPluginItem(const std::string &path) {
	auto plugin=factory().makePlugin(_lua);
	
	for(auto it=_pluginSearchPath.cbegin();it!=_pluginSearchPath.cend();it++) {
		plugin->addSearchPath(*it);
	}
	
	try {
		plugin->open(path);
	}
	catch(...) {
		delete plugin;
		throw;
	}
	
// Check whether the plugin is already opened
	for(std::size_t i=0;i<children();i++) {
		auto p=dynamic_cast<SDMPluginLua*>(&child(i));
		if(p&&*p) {
			auto oldPath=Path(p->path()).toAbsolute();
			auto newPath=Path(plugin->path()).toAbsolute();
			if(oldPath==newPath) {
				delete plugin;
				return *p;
			}
		}
	}
	
	return insertChild(plugin);
}

LuaValue LuaBridge::infoTag(const std::string &name) const {
	auto it=_infoTags.find(name);
	if(it!=_infoTags.end()) return it->second;
	throw std::runtime_error("Unrecognized argument: \""+name+"\"");
}

void LuaBridge::setInfoTag(const std::string &name,const LuaValue &value) {
	_infoTags[name]=value;
}

std::function<int(LuaServer&)> LuaBridge::enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
	switch(i) {
	case 0:
		strName="openplugin";
		return std::bind(&LuaBridge::LuaMethod_openplugin,this,_1);
	case 1:
		strName="plugins";
		return std::bind(&LuaBridge::LuaMethod_plugins,this,_1);
	case 2:
		strName="findobject";
		return std::bind(&LuaBridge::LuaMethod_findobject,this,_1);
	case 3:
		strName="info";
		return std::bind(&LuaBridge::LuaMethod_info,this,_1);
	case 4:
		strName="path";
		return std::bind(&LuaBridge::LuaMethod_path,this,_1);
	case 5:
		strName="lock";
		return std::bind(&LuaBridge::LuaMethod_lock,this,_1);
	default:
		return std::function<int(LuaServer&)>();
	}
}

int LuaBridge::LuaMethod_openplugin(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("openplugin() method takes 1 argument");
	SDMPluginLua &plugin=addPluginItem(lua.argv(0).toString());
	lua.pushValue(plugin.luaHandle());
	return 1;
}

int LuaBridge::LuaMethod_plugins(LuaServer &lua) {
	if(lua.argc()==0) {
// Return number of plugins
		lua.pushValue(lua_Integer(children()));
		return 1;
	}
	if(lua.argc()!=1) throw std::runtime_error("plugins() method takes 0 or 1 arguments");
// Return the requested plugin
	auto n=static_cast<std::size_t>(lua.argv(0).toInteger()-1);
	SDMPluginLua &plugin=child(n).cast<SDMPluginLua>();
	lua.pushValue(plugin.luaHandle());
	return 1;
}

int LuaBridge::LuaMethod_findobject(LuaServer &lua) {
	if(lua.argc()!=1&&lua.argc()!=2) throw std::runtime_error("findobject() method takes 1 or 2 arguments");
	std::string name=lua.argv(0).toString();
	std::string type;
	if(lua.argc()>=2) type=lua.argv(1).toString();
	
	LuaValue handle;
	
	auto obj=findObject(this,name,type);
	if(obj) {
		if(auto plugin=dynamic_cast<SDMPluginLua*>(obj)) {
			handle=plugin->luaHandle();
		}
		else if(auto device=dynamic_cast<SDMDeviceLua*>(obj)) {
			handle=device->luaHandle();
		}
		else if(auto channel=dynamic_cast<SDMChannelLua*>(obj)) {
			handle=channel->luaHandle();
		}
		else if(auto source=dynamic_cast<SDMSourceLua*>(obj)) {
			handle=source->luaHandle();
		}
	}
	
	lua.pushValue(handle);
	return 1;
}

int LuaBridge::LuaMethod_info(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("info() method takes 1 argument");
	lua.pushValue(infoTag(lua.argv(0).toString()));
	return 1;
}

int LuaBridge::LuaMethod_path(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("path() method takes 1 argument");
	
	auto const &arg=lua.argv(0).toString();
	
	if(arg=="currentdir") {
		lua.pushValue(Path::current().str());
	}
	else if(arg=="homedir") {
		lua.pushValue(Path::home().str());
	}
	else if(arg=="currentscript"||arg=="currentscriptdir") {
		auto const &chunkName=lua.currentChunkName();
		if(chunkName.empty()||chunkName[0]!='@') lua.pushValue(LuaValue());
		else {
			Path path(chunkName.substr(1));
			path=path.toAbsolute();
			if(arg=="currentscriptdir") path=path.up();
			lua.pushValue(path.str());
		}
	}
	else if(arg=="program") {
		lua.pushValue(Path::exePath().toAbsolute().str());
	}
	else if(arg=="installprefix") {
		lua.pushValue(Config::installPrefix().toAbsolute().str());
	}
	else if(arg=="bindir") {
		lua.pushValue(Config::binDir().toAbsolute().str());
	}
	else if(arg=="pluginsdir") {
		lua.pushValue(Config::pluginsDir().toAbsolute().str());
	}
	else if(arg=="luamodulesdir") {
		lua.pushValue(Config::luaModulesDir().toAbsolute().str());
	}
	else if(arg=="luacmodulesdir") {
		lua.pushValue(Config::luaCModulesDir().toAbsolute().str());
	}
	else if(arg=="qtdir") {
		lua.pushValue(Config::qtDir().toAbsolute().str());
	}
	else if(arg=="docdir") {
		lua.pushValue(Config::docDir().toAbsolute().str());
	}
	else if(arg=="translationsdir") {
		lua.pushValue(Config::translationsDir().toAbsolute().str());
	}
	else if(arg=="scriptsdir") {
		lua.pushValue(Config::scriptsDir().toAbsolute().str());
	}
	else if(arg=="datadir") {
		lua.pushValue(Config::dataDir().toAbsolute().str());
	}
	else throw std::runtime_error("Unrecognized argument: \""+arg+"\"");
	
	return 1;
}

int LuaBridge::LuaMethod_sleep(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("sleep() method takes 1 argument");
	std::this_thread::sleep_for(std::chrono::milliseconds(lua.argv(0).toInteger()));
	return 0;
}

int LuaBridge::LuaMethod_time(LuaServer &lua) {
	if(lua.argc()!=0) throw std::runtime_error("time() method doesn't take arguments");
	auto tp=std::chrono::steady_clock::now();
	auto d=std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
	lua.pushValue(lua_Integer(d.count()));
	return 1;
}

int LuaBridge::LuaMethod_lock(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("lock() method takes 1 argument");
	if(callbackMutex()==nullptr) return 0; // Mutex is not set - do nothing
	if(lua.argv(0).toBoolean()) { // lock
		if(*_lockCnt==0) { // if not already locked
			callbackMutex()->lock(); // lock mutex
// Add finalizer to be called after Lua chunk completion
			lua.addFinalizer(std::bind(lockFinalizer,callbackMutex(),_lockCnt));
		}
		(*_lockCnt)++;
	}
	else { // unlock
		if(*_lockCnt<=0) throw std::runtime_error("Mutex is not locked");
		(*_lockCnt)--;
		if(*_lockCnt==0) callbackMutex()->unlock();
	}
	return 0;
}

void LuaBridge::lockFinalizer(callback_mutex_t *m,const std::shared_ptr<int> &cnt) {
	if(*cnt>0) {
		m->unlock();
		*cnt=0;
	}
}

TreeItem *LuaBridge::findObject(TreeItem *root,const std::string &name,const std::string &type) {
	if(!root) return nullptr;
	
	if(type=="Plugin"||type=="") {
		auto r=dynamic_cast<SDMPluginLua*>(root);
		try {
			if(r&&r->getProperty("Name")==name) return root;
		}
		catch(std::exception &) {}
	}
	if(type=="Device"||type=="") {
		auto r=dynamic_cast<SDMDeviceLua*>(root);
		try {
			if(r&&r->getProperty("Name")==name) return root;
		}
		catch(std::exception &) {}
	}
	if(type=="Channel"||type=="") {
		auto r=dynamic_cast<SDMChannelLua*>(root);
		try {
			if(r&&r->getProperty("Name")==name) return root;
		}
		catch(std::exception &) {}
	}
	if(type=="Source"||type=="") {
		auto r=dynamic_cast<SDMSourceLua*>(root);
		try {
			if(r&&r->getProperty("Name")==name) return root;
		}
		catch(std::exception &) {}
	}
	
	for(std::size_t i=0;i<root->children();i++) {
		auto r=findObject(&root->child(i),name,type);
		if(r) return r;
	}
	
	return nullptr;
}

/*
 * SDMPluginLua members
 */

SDMPluginLua::SDMPluginLua(LuaServer &l): _lua(l) {}

SDMPluginLua::~SDMPluginLua() {
	_lua.unregisterObject(*this);
}

// We don't register object in the constructor because registering involves virtual function call
const LuaValue &SDMPluginLua::luaHandle() {
	if(_handle.type()==LuaValue::Nil) _handle=_lua.registerObject(*this);
	return _handle;
}

BridgeFactory &SDMPluginLua::factory() const {
	return BridgeFactory::global;
}

void SDMPluginLua::open(const std::string &strFileName) {
	SDMPlugin::open(strFileName);
	
	std::string filename=strFileName;
	auto sep=filename.find_last_of("\\/");
	if(sep!=std::string::npos) filename=filename.substr(sep+1,std::string::npos);
	
	setName(getProperty("Name",filename));
}

void SDMPluginLua::close() {
	delete this;
}

SDMDeviceLua &SDMPluginLua::addDeviceItem(int iDev) {
	auto device=factory().makeDevice(_lua);
	try {
		device->open(*this,iDev);
	}
	catch(...) {
		delete device;
		throw;
	}
	
// Check whether we already have an item with the same handle
	for(std::size_t i=0;i<children();i++) {
		auto dev=dynamic_cast<SDMDeviceLua*>(&child(i));
		if(dev&&*dev&&device->handle()==dev->handle()) {
			delete device;
			return *dev;
		}
	}
	
	insertChild(device);
	
// Auto-open channels and sources (if requested)
	if(device->getProperty("AutoOpenChannels","no")=="open") {
		try {
			const auto &channelList=device->listProperties("Channels");
			for(int i=0;i<static_cast<int>(channelList.size());i++) {
				device->addChannelItem(i);
			}
		}
		catch(std::exception &) {}
	}
	
	if(device->getProperty("AutoOpenSources","no")=="open") {
		try {
			const auto &sourceList=device->listProperties("Sources");
			for(int i=0;i<static_cast<int>(sourceList.size());i++) {
				device->addSourceItem(i);
			}
		}
		catch(std::exception &) {}
	}

	return *device;
}

std::function<int(LuaServer&)> SDMPluginLua::enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
	switch(i) {
	case 0:
		strName="path";
		return std::bind(&SDMPluginLua::LuaMethod_path,this,_1);
	case 1:
		strName="close";
		return std::bind(&SDMPluginLua::LuaMethod_close,this,_1);
	case 2:
		strName="opendevice";
		return std::bind(&SDMPluginLua::LuaMethod_opendevice,this,_1);
	case 3:
		strName="devices";
		return std::bind(&SDMPluginLua::LuaMethod_devices,this,_1);
	default:
		return enumeratePropertyMethods(i-4,strName,upvalues);
	}
}

int SDMPluginLua::LuaMethod_path(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("path() method doesn't take arguments");
	lua.pushValue(path());
	return 1;
}

int SDMPluginLua::LuaMethod_close(LuaServer &lua) {
	if(lua.argc()!=0) throw std::runtime_error("close() method doesn't take arguments");
	close();
	return 0;
}

int SDMPluginLua::LuaMethod_opendevice(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("opendevice() method takes 1 argument");
	int iDev=static_cast<int>(lua.argv(0).toInteger());
	SDMDeviceLua &device=addDeviceItem(iDev);
	lua.pushValue(device.luaHandle());
	return 1;
}

int SDMPluginLua::LuaMethod_devices(LuaServer &lua) {
	if(lua.argc()==0) {
// Return number of devices
		lua.pushValue(lua_Integer(children()));
		return 1;
	}
	if(lua.argc()!=1) throw std::runtime_error("devices() method takes 0 or 1 arguments");
// Return the requested device
	auto n=static_cast<std::size_t>(lua.argv(0).toInteger()-1);
	SDMDeviceLua &device=child(n).cast<SDMDeviceLua>();
	lua.pushValue(device.luaHandle());
	return 1;
}

/*
 * SDMDeviceLua members
 */

SDMDeviceLua::SDMDeviceLua(LuaServer &l): _lua(l) {}

SDMDeviceLua::~SDMDeviceLua() {
	_lua.unregisterObject(*this);
}

// We don't register object in the constructor because registering involves virtual function call
const LuaValue &SDMDeviceLua::luaHandle() {
	if(_handle.type()==LuaValue::Nil) _handle=_lua.registerObject(*this);
	return _handle;
}

BridgeFactory &SDMDeviceLua::factory() const {
	return BridgeFactory::global;
}

void SDMDeviceLua::open(const SDMPlugin &pl,int iDev) {
	SDMDevice::open(pl,iDev);
	setName(getProperty("Name","Device "+std::to_string(iDev)));
}

void SDMDeviceLua::close() {
	delete this;
}

void SDMDeviceLua::connect() {
	SDMDevice::connect();
// Auto-open channels and sources (if requested)
	if(getProperty("AutoOpenChannels","no")=="connect") {
		try {
			const auto &channelList=listProperties("Channels");
			for(int i=0;i<static_cast<int>(channelList.size());i++) {
				addChannelItem(i);
			}
		}
		catch(std::exception &) {}
	}
	
	if(getProperty("AutoOpenSources","no")=="connect") {
		try {
			const auto &sourceList=listProperties("Sources");
			for(int i=0;i<static_cast<int>(sourceList.size());i++) {
				addSourceItem(i);
			}
		}
		catch(std::exception &) {}
	}
}

SDMChannelLua &SDMDeviceLua::addChannelItem(int iChannel) {
	auto const &str=getProperty("AutoOpenChannels","no");
	if(str=="open"||str=="connect") { // check if the channel is already opened
		for(std::size_t i=0;i<children();i++) {
			if(SDMChannelLua *ch=dynamic_cast<SDMChannelLua*>(&child(i))) {
				if(ch->id()==iChannel) return *ch;
			}
		}
	}
	
	auto channel=factory().makeChannel(_lua);
	
	try {
		channel->open(*this,iChannel);
	}
	catch(...) {
		delete channel;
		throw;
	}
	
// Check whether we already have an item with the same handle
	for(std::size_t i=0;i<children();i++) {
		auto ch=dynamic_cast<SDMChannelLua*>(&child(i));
		if(ch&&*ch&&channel->handle()==ch->handle()) {
			delete channel;
			return *ch;
		}
	}
	
	insertChild(channel);
	
	return *channel;
}

SDMSourceLua &SDMDeviceLua::addSourceItem(int iSource) {
	auto const &str=getProperty("AutoOpenSources","no");
	if(str=="open"||str=="connect") { // check if the source is already opened
		for(std::size_t i=0;i<children();i++) {
			if(SDMSourceLua *src=dynamic_cast<SDMSourceLua*>(&child(i))) {
				if(src->id()==iSource) return *src;
			}
		}
	}
	
	auto source=factory().makeSource(_lua);
	
	try {
		source->open(*this,iSource);
	}
	catch(...) {
		delete source;
		throw;
	}
	
// Check whether we already have an item with the same handle
	for(std::size_t i=0;i<children();i++) {
		auto src=dynamic_cast<SDMSourceLua*>(&child(i));
		if(src&&*src&&source->handle()==src->handle()) {
			delete source;
			return *src;
		}
	}
	
	insertChild(source);
	
	return *source;
}

std::function<int(LuaServer&)> SDMDeviceLua::enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
	switch(i) {
	case 0:
		strName="id";
		return std::bind(&SDMDeviceLua::LuaMethod_id,this,_1);
	case 1:
		strName="close";
		return std::bind(&SDMDeviceLua::LuaMethod_close,this,_1);
	case 2:
		strName="connect";
		return std::bind(&SDMDeviceLua::LuaMethod_connect,this,_1);
	case 3:
		strName="disconnect";
		return std::bind(&SDMDeviceLua::LuaMethod_disconnect,this,_1);
	case 4:
		strName="isconnected";
		return std::bind(&SDMDeviceLua::LuaMethod_isconnected,this,_1);
	case 5:
		strName="openchannel";
		return std::bind(&SDMDeviceLua::LuaMethod_openchannel,this,_1);
	case 6:
		strName="channels";
		return std::bind(&SDMDeviceLua::LuaMethod_channels,this,_1);
	case 7:
		strName="opensource";
		return std::bind(&SDMDeviceLua::LuaMethod_opensource,this,_1);
	case 8:
		strName="sources";
		return std::bind(&SDMDeviceLua::LuaMethod_sources,this,_1);
	default:
		return enumeratePropertyMethods(i-9,strName,upvalues);
	}
}

int SDMDeviceLua::LuaMethod_id(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("id() method doesn't take arguments");
	lua.pushValue(lua_Integer(id()));
	return 1;
}

int SDMDeviceLua::LuaMethod_close(LuaServer &lua) {
	if(lua.argc()!=0) throw std::runtime_error("close() method doesn't take arguments");
	close();
	return 0;
}

int SDMDeviceLua::LuaMethod_connect(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("connect() method doesn't take arguments");
	connect();
	return 0;
}

int SDMDeviceLua::LuaMethod_disconnect(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("disconnect() method doesn't take arguments");
	disconnect();
	return 0;
}

int SDMDeviceLua::LuaMethod_isconnected(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("isconnected() method doesn't take arguments");
	lua.pushValue(isConnected());
	return 1;
}

int SDMDeviceLua::LuaMethod_openchannel(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("openchannel() method takes 1 argument");
	int ch=static_cast<int>(lua.argv(0).toInteger());
	SDMChannelLua &channel=addChannelItem(ch);
	lua.pushValue(channel.luaHandle());
	return 1;
}

int SDMDeviceLua::LuaMethod_channels(LuaServer &lua) {
	std::vector<SDMChannelLua*> channels;
	for(std::size_t i=0;i<children();i++)
		if(auto ch=dynamic_cast<SDMChannelLua*>(&child(i)))
			channels.push_back(ch);
	
	if(lua.argc()==0) {
// Return number of channels
		lua.pushValue(static_cast<lua_Integer>(channels.size()));
		return 1;
	}
	if(lua.argc()!=1) throw std::runtime_error("channels() method takes 0 or 1 arguments");
// Return the requested channel
	auto n=static_cast<std::size_t>(lua.argv(0).toInteger()-1);
	if(n>=channels.size()) throw std::runtime_error("Channel with such an index doesn't exist");
	lua.pushValue(channels[n]->luaHandle());
	return 1;
}

int SDMDeviceLua::LuaMethod_opensource(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("opensource() method takes 1 argument");
	int src=static_cast<int>(lua.argv(0).toInteger());
	SDMSourceLua &source=addSourceItem(src);
	lua.pushValue(source.luaHandle());
	return 1;
}

int SDMDeviceLua::LuaMethod_sources(LuaServer &lua) {
	std::vector<SDMSourceLua*> sources;
	for(std::size_t i=0;i<children();i++)
		if(auto ch=dynamic_cast<SDMSourceLua*>(&child(i)))
			sources.push_back(ch);
	
	if(lua.argc()==0) {
// Return number of sources
		lua.pushValue(static_cast<lua_Integer>(sources.size()));
		return 1;
	}
	if(lua.argc()!=1) throw std::runtime_error("sources() method takes 0 or 1 arguments");
// Return the requested source
	auto n=static_cast<std::size_t>(lua.argv(0).toInteger()-1);
	if(n>=sources.size()) throw std::runtime_error("Source with such an index doesn't exist");
	lua.pushValue(sources[n]->luaHandle());
	return 1;
}

/*
 * SDMChannelLua members
 */

SDMChannelLua::SDMChannelLua(LuaServer &l): _lua(l) {}

SDMChannelLua::~SDMChannelLua() {
	_lua.unregisterObject(*this);
}

// We don't register object in the constructor because registering involves virtual function call
const LuaValue &SDMChannelLua::luaHandle() {
	if(_handle.type()==LuaValue::Nil) _handle=_lua.registerObject(*this);
	return _handle;
}

BridgeFactory &SDMChannelLua::factory() const {
	return BridgeFactory::global;
}

void SDMChannelLua::open(const SDMDevice &d,int ch) {
	SDMChannel::open(d,ch);
	setName(getProperty("Name","Channel "+std::to_string(ch)));
}

void SDMChannelLua::close() {
	delete this;
}

std::function<int(LuaServer&)> SDMChannelLua::enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
	switch(i) {
	case 0:
		strName="id";
		return std::bind(&SDMChannelLua::LuaMethod_id,this,_1);
	case 1:
		strName="close";
		return std::bind(&SDMChannelLua::LuaMethod_close,this,_1);
	case 2:
		strName="writereg";
		return std::bind(&SDMChannelLua::LuaMethod_writereg,this,_1);
	case 3:
		strName="readreg";
		return std::bind(&SDMChannelLua::LuaMethod_readreg,this,_1);
	case 4:
		strName="writefifo";
		return std::bind(&SDMChannelLua::LuaMethod_writefifo,this,_1);
	case 5:
		strName="readfifo";
		return std::bind(&SDMChannelLua::LuaMethod_readfifo,this,_1);
	case 6:
		strName="writemem";
		return std::bind(&SDMChannelLua::LuaMethod_writemem,this,_1);
	case 7:
		strName="readmem";
		return std::bind(&SDMChannelLua::LuaMethod_readmem,this,_1);
	default:
		return enumeratePropertyMethods(i-8,strName,upvalues);
	}
}

int SDMChannelLua::LuaMethod_id(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("id() method doesn't take arguments");
	lua.pushValue(lua_Integer(id()));
	return 1;
}

int SDMChannelLua::LuaMethod_close(LuaServer &lua) {
	if(lua.argc()!=0) throw std::runtime_error("close() method doesn't take arguments");
	close();
	return 0;
}

int SDMChannelLua::LuaMethod_writereg(LuaServer &lua) {
	if(lua.argc()!=2) throw std::runtime_error("writereg() method takes 2 arguments");
	writeReg((sdm_addr_t)lua.argv(0).toInteger(),(sdm_reg_t)lua.argv(1).toInteger());
	return 0;
}

int SDMChannelLua::LuaMethod_readreg(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("readreg() method takes 1 argument");
	lua.pushValue(lua_Integer(readReg((sdm_addr_t)lua.argv(0).toInteger())));
	return 1;
}

int SDMChannelLua::LuaMethod_writefifo(LuaServer &lua) {
	if(lua.argc()<2&&lua.argc()>3) throw std::runtime_error("writefifo() method takes 2-3 arguments");
	
	auto const addr=static_cast<sdm_addr_t>(lua.argv(0).toInteger());
	auto const &t=lua.argv(1);
	if(t.type()!=LuaValue::Table) throw std::runtime_error("writefifo() 2nd argument must be of table type");
	
	Flags flags=Normal;
	int modeflags=0;
	if(lua.argc()==3) {
		const std::string &str=lua.argv(2).toString();
		if(str.find("all")!=std::string::npos) modeflags++;
		if(str.find("part")!=std::string::npos) {
			flags|=AllowPartial;
			modeflags++;
		}
		if(str.find("nb")!=std::string::npos) {
			flags|=NonBlocking;
			modeflags++;
		}
		if(str.find("start")!=std::string::npos) flags|=StartOfPacket;
	}
	
	if(modeflags>1) throw std::runtime_error("Only one of \"all\", \"part\" and \"nb\" flags can be specified");
	
	std::vector<sdm_reg_t> data;
	data.reserve(t.size());
	for(auto const &v: t.table()) data.push_back(static_cast<sdm_reg_t>(v.second.toInteger()));
	
	int r=writeFIFO(addr,data.data(),data.size(),flags);
	
	if(r==WouldBlock) lua.pushValue(LuaValue());
	else lua.pushValue(lua_Integer(r));
	
	return 1;
}

int SDMChannelLua::LuaMethod_readfifo(LuaServer &lua) {
	if(lua.argc()<2&&lua.argc()>3) throw std::runtime_error("readfifo() method takes 2-3 arguments");
	
	auto const addr=static_cast<sdm_addr_t>(lua.argv(0).toInteger());
	auto const n=static_cast<std::size_t>(lua.argv(1).toInteger());
	
	Flags flags=Normal;
	int modeflags=0;
	if(lua.argc()==3) {
		const std::string &str=lua.argv(2).toString();
		if(str.find("all")!=std::string::npos) modeflags++;
		if(str.find("part")!=std::string::npos) {
			flags|=AllowPartial;
			modeflags++;
		}
		if(str.find("nb")!=std::string::npos) {
			flags|=NonBlocking;
			modeflags++;
		}
		if(str.find("next")!=std::string::npos) flags|=NextPacket;
	}
	
	if(modeflags>1) throw std::runtime_error("Only one of \"all\", \"part\" and \"nb\" flags can be specified");

	std::vector<sdm_reg_t> data;
	
	sdm_reg_t *buf=nullptr;
	if(n>0) {
		data.resize(n);
		buf=data.data();
	}
	int r=readFIFO(addr,buf,n,flags);
	
	if(r==WouldBlock) {
		lua.pushValue(LuaValue());
		return 1;
	}
	
	data.resize(r);
	
	LuaValue t;
	auto &arr=t.newarray();
	arr.reserve(data.size());
	for(auto const &v: data) arr.emplace_back(static_cast<lua_Integer>(v));
	lua.pushValue(t);	
	return 1;
}

int SDMChannelLua::LuaMethod_writemem(LuaServer &lua) {
	if(lua.argc()!=2) throw std::runtime_error("writemem() method takes 2 arguments");
	
	auto const addr=static_cast<sdm_reg_t>(lua.argv(0).toInteger());
	auto const &data=lua.argv(1,true); // get argument as array
	if(data.type()!=LuaValue::Array) throw std::runtime_error("writemem() 2nd argument must be of table type");
	
	auto const &arr=data.array();
	std::vector<sdm_reg_t> v(arr.size());
	for(std::size_t i=0;i<arr.size();i++) v[i]=static_cast<sdm_reg_t>(arr[i].toInteger());
	
	writeMem(addr,v.data(),v.size());
	
	return 0;
}

int SDMChannelLua::LuaMethod_readmem(LuaServer &lua) {
	if(lua.argc()!=2) throw std::runtime_error("readmem() method takes 2 arguments");
	
	auto const addr=static_cast<sdm_reg_t>(lua.argv(0).toInteger());
	auto const n=static_cast<std::size_t>(lua.argv(1).toInteger());
	if(n==0) return 0;
	
	std::vector<sdm_reg_t> data(n);
	readMem(addr,data.data(),n);
	
	LuaValue res;
	res.newarray();
	auto &v=res.array();
	v.reserve(n);
	for(std::size_t i=0;i<n;i++) v.emplace_back(static_cast<lua_Integer>(data[i]));
	
	lua.pushValue(res);

	return 1;
}

/*
 * SDMSourceLua members
 */

SDMSourceLua::SDMSourceLua(LuaServer &l): _lua(l) {}

SDMSourceLua::~SDMSourceLua() {
	_lua.unregisterObject(*this);
}

// We don't register object in the constructor because registering involves virtual function call
const LuaValue &SDMSourceLua::luaHandle() {
	if(_handle.type()==LuaValue::Nil) _handle=_lua.registerObject(*this);
	return _handle;
}

BridgeFactory &SDMSourceLua::factory() const {
	return BridgeFactory::global;
}

void SDMSourceLua::open(const SDMDevice &d,int ch) {
	SDMSource::open(d,ch);
	setName(getProperty("Name","Source "+std::to_string(ch)));
}

void SDMSourceLua::close() {
	delete this;
}

std::function<int(LuaServer&)> SDMSourceLua::enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
	switch(i) {
	case 0:
		strName="id";
		return std::bind(&SDMSourceLua::LuaMethod_id,this,_1);
	case 1:
		strName="close";
		return std::bind(&SDMSourceLua::LuaMethod_close,this,_1);
	case 2:
		strName="selectreadstreams";
		return std::bind(&SDMSourceLua::LuaMethod_selectreadstreams,this,_1);
	case 3:
		strName="readstream";
		return std::bind(&SDMSourceLua::LuaMethod_readstream,this,_1);
	case 4:
		strName="readnextpacket";
		return std::bind(&SDMSourceLua::LuaMethod_readnextpacket,this,_1);
	case 5:
		strName="discardpackets";
		return std::bind(&SDMSourceLua::LuaMethod_discardpackets,this,_1);
	case 6:
		strName="readstreamerrors";
		return std::bind(&SDMSourceLua::LuaMethod_readstreamerrors,this,_1);
	default:
		return enumeratePropertyMethods(i-7,strName,upvalues);
	}
}

int SDMSourceLua::LuaMethod_id(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("id() method doesn't take arguments");
	lua.pushValue(lua_Integer(id()));
	return 1;
}

int SDMSourceLua::LuaMethod_close(LuaServer &lua) {
	if(lua.argc()!=0) throw std::runtime_error("close() method doesn't take arguments");
	close();
	return 0;
}

int SDMSourceLua::LuaMethod_selectreadstreams(LuaServer &lua) {
	if(lua.argc()<1||lua.argc()>3) throw std::runtime_error("selectreadstreams() method takes 1-3 arguments");
	
	std::vector<int> streams;
	const LuaValue &t=lua.argv(0);
	if(t.type()!=LuaValue::Table) throw std::runtime_error("Wrong 1st argument type (table expected)");
	for(auto it=t.table().cbegin();it!=t.table().cend();it++) {
		streams.push_back(static_cast<int>(it->second.toInteger()));
	}
	
	std::size_t packets=0;
	if(lua.argc()>=2) packets=static_cast<std::size_t>(lua.argv(1).toInteger());
	
	int df=1;
	if(lua.argc()==3) df=static_cast<int>(lua.argv(2).toInteger());
	
	selectReadStreams(streams,packets,df);
	
	return 0;
}

int SDMSourceLua::LuaMethod_readstream(LuaServer &lua) {
	if(lua.argc()!=2&&lua.argc()!=3) throw std::runtime_error("readstream() method takes 2-3 arguments");
	
	const int stream=static_cast<int>(lua.argv(0).toInteger());
	auto const n=static_cast<std::size_t>(lua.argv(1).toInteger());
	
	Flags f=Normal;
	if(lua.argc()==3) {
		auto const &str=lua.argv(2).toString();
		if(str=="all");
		else if(str=="nb") f=NonBlocking;
		else if(str=="part") f=AllowPartial;
		else throw std::runtime_error("Bad flag");
	}
	
	if(n==0) {
		int r=readStream(stream,nullptr,0,f);
		lua.pushValue(static_cast<lua_Integer>(r));
	}
	else {
		std::vector<sdm_sample_t> data(n);
		int r=readStream(stream,data.data(),n,f);
		if(r==WouldBlock) {
			lua.pushValue(LuaValue());
		}
		else {
			LuaValue t;
			auto &arr=t.newarray();
			if(r>0) arr.reserve(r);
			for(int i=0;i<r;i++) {
				arr.emplace_back(static_cast<lua_Number>(data[i]));
			}
			lua.pushValue(t);
		}
	}
	
	return 1;
}

int SDMSourceLua::LuaMethod_readnextpacket(LuaServer &lua) {
	if(lua.argc()!=0) throw std::runtime_error("readnextpacket() doesn't take arguments");
	readNextPacket();
	return 0;
}

int SDMSourceLua::LuaMethod_discardpackets(LuaServer &lua) {
	if(lua.argc()!=0) throw std::runtime_error("discardpackets() doesn't take arguments");
	discardPackets();
	return 0;
}

int SDMSourceLua::LuaMethod_readstreamerrors(LuaServer &lua) {
	if(lua.argc()!=0) throw std::runtime_error("readstreamerrors() doesn't take arguments");
	int r=readStreamErrors();
	lua.pushValue(static_cast<lua_Integer>(r));
	return 1;
}
