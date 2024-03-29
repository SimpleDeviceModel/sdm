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
 * This module provides an implementation of the SDMChannel class.
 * The real work is done by an uncopyable SDMChannelImpl class.
 * SDMChannel stores a shared pointer to an SDMChannelImpl instance.
 */

#include "sdmplug.h"

#include <stdexcept>

/*
 * SDMChannelImpl definition
 */

class SDMChannelImpl {
	SDMDevice _device;
	void *_hChannel;
	int _id;
	const SDMImport &_pf;
	
public:
	SDMChannelImpl(const SDMDevice &d,int ch);
	SDMChannelImpl(const SDMChannelImpl &)=delete;
	~SDMChannelImpl();
	
	SDMChannelImpl &operator=(const SDMChannelImpl &right)=delete;
	
	void *handle() const {return _hChannel;}
	SDMPlugin &plugin() {return _device.plugin();}
	const SDMPlugin &plugin() const {return _device.plugin();}
	SDMDevice &device() {return _device;}
	const SDMDevice &device() const {return _device;}
	
	int getPropertyAPI(const char *name,char *buf,std::size_t n);
	int setPropertyAPI(const char *name,const char *value);
	
	void writeReg(sdm_addr_t addr,sdm_reg_t data);
	sdm_reg_t readReg(sdm_addr_t addr);
	void writeFIFO(sdm_addr_t addr,const sdm_reg_t *data,std::size_t n);
	void readFIFO(sdm_addr_t addr,sdm_reg_t *data,std::size_t n);
	void writeMem(sdm_addr_t addr,const sdm_reg_t *data,std::size_t n);
	void readMem(sdm_addr_t addr,sdm_reg_t *data,std::size_t n);
	
	int id() const {return _id;}
};

/*
 * SDMChannelImpl members
 */

SDMChannelImpl::SDMChannelImpl(const SDMDevice &d,int ch): _device(d),_pf(d.plugin().functions()) {
	if(!_device) throw std::runtime_error("Device is not opened");
	if(!_pf.supportChannels) throw std::runtime_error("This plugin doesn't support control channels");
	_hChannel=_pf.ptrOpenChannel(_device.handle(),ch);
	if(!_hChannel) throw sdmplugin_error("sdmOpenChannel");
	_id=ch;
}

SDMChannelImpl::~SDMChannelImpl() {
	_pf.ptrCloseChannel(_hChannel);
}

int SDMChannelImpl::getPropertyAPI(const char *name,char *buf,std::size_t n) {
	return _pf.ptrGetChannelProperty(_hChannel,name,buf,n);
}

int SDMChannelImpl::setPropertyAPI(const char *name,const char *value) {
	return _pf.ptrSetChannelProperty(_hChannel,name,value);
}

void SDMChannelImpl::writeReg(sdm_addr_t addr,sdm_reg_t data) {
	int r=_pf.ptrWriteReg(_hChannel,addr,data);
	if(r) throw sdmplugin_error("sdmWriteReg",r);
}

sdm_reg_t SDMChannelImpl::readReg(sdm_addr_t addr) {
	sdm_reg_t val;
	int err;
	
	val=_pf.ptrReadReg(_hChannel,addr,&err);
	if(err) throw sdmplugin_error("sdmReadReg",err);
	return val;
}

void SDMChannelImpl::writeFIFO(sdm_addr_t addr,const sdm_reg_t *data,std::size_t n) {
	int r=_pf.ptrWriteFIFO(_hChannel,addr,data,n,0);
	if(r<0) throw sdmplugin_error("sdmWriteFIFO",r);
}

void SDMChannelImpl::readFIFO(sdm_addr_t addr,sdm_reg_t *data,std::size_t n) {
	int r=_pf.ptrReadFIFO(_hChannel,addr,data,n,0);
	if(r<0) throw sdmplugin_error("sdmReadFIFO",r);
}

void SDMChannelImpl::writeMem(sdm_addr_t addr,const sdm_reg_t *data,std::size_t n) {
	int r=_pf.ptrWriteMem(_hChannel,addr,data,n);
	if(r) throw sdmplugin_error("sdmWriteMem",r);
}

void SDMChannelImpl::readMem(sdm_addr_t addr,sdm_reg_t *data,std::size_t n) {
	int r=_pf.ptrReadMem(_hChannel,addr,data,n);
	if(r) throw sdmplugin_error("sdmReadMem",r);
}

/*
 * SDMChannel members
 */

SDMChannel::SDMChannel(const SDMDevice &d,int ch) {
	open(d,ch);
}

inline SDMChannelImpl &SDMChannel::impl() {
	if(!_impl) throw std::runtime_error("Channel not opened");
	return *_impl;
}

inline const SDMChannelImpl &SDMChannel::impl() const {
	if(!_impl) throw std::runtime_error("Channel not opened");
	return *_impl;
}

int SDMChannel::getPropertyAPI(const char *name,char *buf,std::size_t n) {
	return impl().getPropertyAPI(name,buf,n);
}

int SDMChannel::setPropertyAPI(const char *name,const char *value) {
	return impl().setPropertyAPI(name,value);
}

void *SDMChannel::handle() const {
	return impl().handle();
}

SDMPlugin &SDMChannel::plugin() {
	return impl().plugin();
}

const SDMPlugin &SDMChannel::plugin() const {
	return impl().plugin();
}

SDMDevice &SDMChannel::device() {
	return impl().device();
}

const SDMDevice &SDMChannel::device() const {
	return impl().device();
}

void SDMChannel::open(const SDMDevice &d,int ch) {
	_impl=std::make_shared<SDMChannelImpl>(d,ch);
}

void SDMChannel::close() {
	_impl.reset();
}

void SDMChannel::writeReg(sdm_addr_t addr,sdm_reg_t data) {
	impl().writeReg(addr,data);
}

sdm_reg_t SDMChannel::readReg(sdm_addr_t addr) {
	return impl().readReg(addr);
}

void SDMChannel::writeFIFO(sdm_addr_t addr,const sdm_reg_t *data,std::size_t n) {
	return impl().writeFIFO(addr,data,n);
}

void SDMChannel::readFIFO(sdm_addr_t addr,sdm_reg_t *data,std::size_t n) {
	return impl().readFIFO(addr,data,n);
}

void SDMChannel::writeMem(sdm_addr_t addr,const sdm_reg_t *data,std::size_t n) {
	impl().writeMem(addr,data,n);
}

void SDMChannel::readMem(sdm_addr_t addr,sdm_reg_t *data,std::size_t n) {
	impl().readMem(addr,data,n);
}

SDMChannel::operator bool() const {
	return _impl.operator bool();
}

int SDMChannel::id() const {
	if(!_impl) return -1;
	return impl().id();
}
