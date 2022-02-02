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
 * This module provides an implementation of the SDMPlugin class.
 */

#include "sdmplug.h"
#include "dirutil.h"
#include "loadablemodule.h"

#include <stdexcept>
#include <sstream>

/*
 * SDMPluginImpl definition
 */

class SDMPluginImpl {
	LoadableModule _lib;
	SDMImport _pf;
	
public:
	explicit SDMPluginImpl(const std::string &strFileName);

	int getPropertyAPI(const char *name,char *buf,std::size_t n);
	int setPropertyAPI(const char *name,const char *value);
	
	const SDMImport &functions() const {return _pf;}
	std::string path() const {return _lib.path();}
private:
	template <typename T> T funcAddr(const std::string &name) {
		return reinterpret_cast<T>(_lib.getAddr(name));
	}
};

/*
 * SDMPluginImpl members
 */

SDMPluginImpl::SDMPluginImpl(const std::string &strFileName): _lib(strFileName) {
// Mandatory functions
	_pf.ptrGetPluginProperty=funcAddr<PtrSdmGetPluginProperty>("sdmGetPluginProperty");
	_pf.ptrSetPluginProperty=funcAddr<PtrSdmSetPluginProperty>("sdmSetPluginProperty");
	
	_pf.ptrOpenDevice=funcAddr<PtrSdmOpenDevice>("sdmOpenDevice");
	_pf.ptrCloseDevice=funcAddr<PtrSdmCloseDevice>("sdmCloseDevice");
	_pf.ptrGetDeviceProperty=funcAddr<PtrSdmGetDeviceProperty>("sdmGetDeviceProperty");
	_pf.ptrSetDeviceProperty=funcAddr<PtrSdmSetDeviceProperty>("sdmSetDeviceProperty");
	_pf.ptrConnect=funcAddr<PtrSdmConnect>("sdmConnect");
	_pf.ptrDisconnect=funcAddr<PtrSdmDisconnect>("sdmDisconnect");
	_pf.ptrGetConnectionStatus=funcAddr<PtrSdmGetConnectionStatus>("sdmGetConnectionStatus");

// Optional channel functions
	_pf.supportChannels=true;
	try {
		_pf.ptrOpenChannel=funcAddr<PtrSdmOpenChannel>("sdmOpenChannel");
		_pf.ptrCloseChannel=funcAddr<PtrSdmCloseChannel>("sdmCloseChannel");
		_pf.ptrGetChannelProperty=funcAddr<PtrSdmGetChannelProperty>("sdmGetChannelProperty");
		_pf.ptrSetChannelProperty=funcAddr<PtrSdmSetChannelProperty>("sdmSetChannelProperty");
		_pf.ptrWriteReg=funcAddr<PtrSdmWriteReg>("sdmWriteReg");
		_pf.ptrReadReg=funcAddr<PtrSdmReadReg>("sdmReadReg");
		_pf.ptrWriteFIFO=funcAddr<PtrSdmWriteFIFO>("sdmWriteFIFO");
		_pf.ptrReadFIFO=funcAddr<PtrSdmReadFIFO>("sdmReadFIFO");
		_pf.ptrWriteMem=funcAddr<PtrSdmWriteMem>("sdmWriteMem");
		_pf.ptrReadMem=funcAddr<PtrSdmReadMem>("sdmReadMem");
	}
	catch(std::exception &) {
		_pf.supportChannels=false;
	}
	
// Optional data source functions
	_pf.supportSources=true;
	try {
		_pf.ptrOpenSource=funcAddr<PtrSdmOpenSource>("sdmOpenSource");
		_pf.ptrCloseSource=funcAddr<PtrSdmCloseSource>("sdmCloseSource");
		_pf.ptrGetSourceProperty=funcAddr<PtrSdmGetSourceProperty>("sdmGetSourceProperty");
		_pf.ptrSetSourceProperty=funcAddr<PtrSdmSetSourceProperty>("sdmSetSourceProperty");
		_pf.ptrSelectReadStreams=funcAddr<PtrSdmSelectReadStreams>("sdmSelectReadStreams");
		_pf.ptrReadStream=funcAddr<PtrSdmReadStream>("sdmReadStream");
		_pf.ptrReadNextPacket=funcAddr<PtrSdmReadNextPacket>("sdmReadNextPacket");
		_pf.ptrDiscardPackets=funcAddr<PtrSdmDiscardPackets>("sdmDiscardPackets");
		_pf.ptrReadStreamErrors=funcAddr<PtrSdmReadStreamErrors>("sdmReadStreamErrors");
	}
	catch(std::exception &) {
		_pf.supportSources=false;
	}
}

int SDMPluginImpl::getPropertyAPI(const char *name,char *buf,std::size_t n) {
	if(!_lib) throw std::runtime_error("Plugin not loaded");
	return _pf.ptrGetPluginProperty(name,buf,n);
}

int SDMPluginImpl::setPropertyAPI(const char *name,const char *value) {
	if(!_lib) throw std::runtime_error("Plugin not loaded");
	return _pf.ptrSetPluginProperty(name,value);
}

/*
 * SDMPlugin members
 */

inline SDMPluginImpl &SDMPlugin::impl() {
	if(!_impl) throw std::runtime_error("Plugin not loaded");
	return *_impl;
}

inline const SDMPluginImpl &SDMPlugin::impl() const {
	if(!_impl) throw std::runtime_error("Plugin not loaded");
	return *_impl;
}

int SDMPlugin::getPropertyAPI(const char *name,char *buf,std::size_t n) {
	return impl().getPropertyAPI(name,buf,n);
}

int SDMPlugin::setPropertyAPI(const char *name,const char *value) {
	return impl().setPropertyAPI(name,value);
}
	
SDMPlugin::SDMPlugin(const std::string &strFileName) {
	open(strFileName);
}
	
void SDMPlugin::open(const std::string &strFileName) {
	std::ostringstream errors;
	
	if(!Path(strFileName).isAbsolute()&&!_pluginSearchPath.empty()) {
// If strFileName is a relative path, it will be resolved relative to SDM plugin path
		for(auto it=_pluginSearchPath.cbegin();it!=_pluginSearchPath.cend();it++) {
			try {
				_impl=std::make_shared<SDMPluginImpl>((Path(*it)+strFileName).str());
				break;
			}
			catch(std::exception &ex) {
				errors<<ex.what();
				if(it+1!=_pluginSearchPath.end()) errors<<std::endl;
			}
		}
		if(!_impl) throw std::runtime_error(errors.str().c_str());
	}
	else {
		_impl=std::make_shared<SDMPluginImpl>(strFileName);
	}
}

void SDMPlugin::close() {
	_impl.reset();
}
	
const SDMImport &SDMPlugin::functions() const {
	return impl().functions();
}
	
SDMPlugin::operator bool() const {
	return _impl.operator bool();
}

std::string SDMPlugin::path() const {
	return impl().path();
}
