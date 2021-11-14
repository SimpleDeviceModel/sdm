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
 * This module provides an implementation of the SDMDevice class.
 * The real work is done by an uncopyable SDMDeviceImpl class.
 * SDMDevice stores a shared pointer to an SDMDeviceImpl instance.
 */

#include "sdmplug.h"

#include <stdexcept>

/*
 * SDMDeviceImpl definition
 */

class SDMDeviceImpl {
	SDMPlugin _plugin;
	void *_hDevice;
	int _id;
	const SDMImport &_pf;
	
public:
	SDMDeviceImpl(const SDMPlugin &pl,int iDev);
	SDMDeviceImpl(const SDMDeviceImpl &)=delete;
	~SDMDeviceImpl();
	
	SDMDeviceImpl &operator=(const SDMDeviceImpl &)=delete;
	
	SDMPlugin &plugin() {return _plugin;}
	const SDMPlugin &plugin() const {return _plugin;}
	
	void *handle() const {return _hDevice;}
	
	int getPropertyAPI(const char *name,char *buf,std::size_t n);
	int setPropertyAPI(const char *name,const char *value);
	
	void connect();
	void disconnect();
	bool isConnected();
	
	int id() const {return _id;}
};

/*
 * SDMDeviceImpl members
 */

SDMDeviceImpl::SDMDeviceImpl(const SDMPlugin &pl,int iDev): _plugin(pl),_pf(pl.functions()) {
	if(!_plugin) throw std::runtime_error("Plugin is not loaded");
	_hDevice=_pf.ptrOpenDevice(iDev);
	if(!_hDevice) throw sdmplugin_error("sdmOpenDevice");
	_id=iDev;
}

SDMDeviceImpl::~SDMDeviceImpl() {
	if(_pf.ptrGetConnectionStatus(_hDevice)!=0) _pf.ptrDisconnect(_hDevice);
	_pf.ptrCloseDevice(_hDevice);
}

int SDMDeviceImpl::getPropertyAPI(const char *name,char *buf,std::size_t n) {
	return _pf.ptrGetDeviceProperty(_hDevice,name,buf,n);
}

int SDMDeviceImpl::setPropertyAPI(const char *name,const char *value) {
	return _pf.ptrSetDeviceProperty(_hDevice,name,value);
}

void SDMDeviceImpl::connect() {
	int r=_pf.ptrConnect(_hDevice);
	if(r) throw sdmplugin_error("sdmConnect",r);
}

void SDMDeviceImpl::disconnect() {
	int r=_pf.ptrDisconnect(_hDevice);
	if(r) throw sdmplugin_error("sdmDisconnect",r);
}

bool SDMDeviceImpl::isConnected() {
	int r=_pf.ptrGetConnectionStatus(_hDevice);
	return (r!=0);
}

/*
 * SDMDevice members
 */

SDMDevice::SDMDevice(const SDMPlugin &pl,int iDev) {
	open(pl,iDev);
}

inline SDMDeviceImpl &SDMDevice::impl() {
	if(!_impl) throw std::runtime_error("Device not opened");
	return *_impl;
}

inline const SDMDeviceImpl &SDMDevice::impl() const {
	if(!_impl) throw std::runtime_error("Device not opened");
	return *_impl;
}

int SDMDevice::getPropertyAPI(const char *name,char *buf,std::size_t n) {
	return impl().getPropertyAPI(name,buf,n);
}

int SDMDevice::setPropertyAPI(const char *name,const char *value) {
	return impl().setPropertyAPI(name,value);
}

void *SDMDevice::handle() const {
	return impl().handle();
}

SDMPlugin &SDMDevice::plugin() {
	return impl().plugin();
}

const SDMPlugin &SDMDevice::plugin() const {
	return impl().plugin();
}

void SDMDevice::open(const SDMPlugin &pl,int iDev) {
	_impl=std::make_shared<SDMDeviceImpl>(pl,iDev);
}

void SDMDevice::close() {
	_impl.reset();
}

void SDMDevice::connect() {
	impl().connect();
}

void SDMDevice::disconnect() {
	impl().disconnect();
}

bool SDMDevice::isConnected() {
	return impl().isConnected();
}

SDMDevice::operator bool() const {
	return _impl.operator bool();
}

int SDMDevice::id() const {
	if(!_impl) return -1;
	return impl().id();
}
