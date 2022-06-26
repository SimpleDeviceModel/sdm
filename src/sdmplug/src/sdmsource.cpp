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
 * This module provides an implementation of the SDMSource class.
 * The real work is done by an uncopyable SDMSourceImpl class.
 * SDMSource stores a shared pointer to an SDMSourceImpl instance.
 */

#include "sdmplug.h"

#include <stdexcept>

const SDMSource::Flags SDMSource::Normal=0;
const SDMSource::Flags SDMSource::NonBlocking=1;
const SDMSource::Flags SDMSource::AllowPartial=2;

const int SDMSource::WouldBlock=SDM_WOULDBLOCK;

/*
 * SDMSourceImpl definition
 */

class SDMSourceImpl {
	SDMDevice _device;
	void *_hSource;
	int _id;
	const SDMImport &_pf;
	
public:
	SDMSourceImpl(const SDMDevice &d,int ch);
	SDMSourceImpl(const SDMSourceImpl &)=delete;
	~SDMSourceImpl();
	
	SDMSourceImpl &operator=(const SDMSourceImpl &right)=delete;
	
	void *handle() const {return _hSource;}
	SDMPlugin &plugin() {return _device.plugin();}
	const SDMPlugin &plugin() const {return _device.plugin();}
	SDMDevice &device() {return _device;}
	const SDMDevice &device() const {return _device;}
	
	int getPropertyAPI(const char *name,char *buf,std::size_t n);
	int setPropertyAPI(const char *name,const char *value);
	
	void selectReadStreams(const std::vector<int> &streams,std::size_t packets,int df);
	int readStream(int stream,sdm_sample_t *data,std::size_t n,SDMSource::Flags flags);
	void readNextPacket();
	void discardPackets();
	int readStreamErrors();
	
	int id() const {return _id;}
};

/*
 * SDMSourceImpl members
 */

SDMSourceImpl::SDMSourceImpl(const SDMDevice &d,int ch): _device(d),_pf(d.plugin().functions()) {
	if(!_device) throw std::runtime_error("Device is not opened");
	if(!_pf.supportSources) throw std::runtime_error("This plugin doesn't support data sources");
	_hSource=_pf.ptrOpenSource(_device.handle(),ch);
	if(!_hSource) throw sdmplugin_error("sdmOpenChannel");
	_id=ch;
}

SDMSourceImpl::~SDMSourceImpl() {
	_pf.ptrCloseSource(_hSource);
}

int SDMSourceImpl::getPropertyAPI(const char *name,char *buf,std::size_t n) {
	return _pf.ptrGetSourceProperty(_hSource,name,buf,n);
}

int SDMSourceImpl::setPropertyAPI(const char *name,const char *value) {
	return _pf.ptrSetSourceProperty(_hSource,name,value);
}

void SDMSourceImpl::selectReadStreams(const std::vector<int> &streams,std::size_t packets,int df) {
	int r=_pf.ptrSelectReadStreams(_hSource,streams.data(),static_cast<int>(streams.size()),packets,df);
	if(r) throw sdmplugin_error("sdmSelectReadStreams",r);
}

int SDMSourceImpl::readStream(int stream,sdm_sample_t *data,std::size_t n,SDMSource::Flags flags) {
	int nb=0;
	if(flags&SDMSource::NonBlocking) nb=1;
	
	if(flags&SDMSource::NonBlocking||flags&SDMSource::AllowPartial||n==0) {
		int r=_pf.ptrReadStream(_hSource,stream,data,n,nb);
		if(r==SDM_WOULDBLOCK) return SDMSource::WouldBlock;
		if(r<0) throw sdmplugin_error("sdmReadStream",r);
		return r;
	}
	else { // read all in a blocking manner
		std::size_t samplesRead=0;
		while(samplesRead<n) {
			int r=_pf.ptrReadStream(_hSource,stream,data+samplesRead,n-samplesRead,nb);
			if(r<0) throw sdmplugin_error("sdmReadStream",r);
			if(r==0) break; // end of packet
			samplesRead+=r;
		}
		return static_cast<int>(samplesRead);
	}
}

void SDMSourceImpl::readNextPacket() {
	int r=_pf.ptrReadNextPacket(_hSource);
	if(r) throw sdmplugin_error("sdmReadNextPacket",r);
}

void SDMSourceImpl::discardPackets() {
	_pf.ptrDiscardPackets(_hSource);
}

int SDMSourceImpl::readStreamErrors() {
	return _pf.ptrReadStreamErrors(_hSource);
}

/*
 * SDMSource members
 */

SDMSource::SDMSource(const SDMDevice &d,int ch) {
	open(d,ch);
}

inline SDMSourceImpl &SDMSource::impl() {
	if(!_impl) throw std::runtime_error("Channel not opened");
	return *_impl;
}

inline const SDMSourceImpl &SDMSource::impl() const {
	if(!_impl) throw std::runtime_error("Channel not opened");
	return *_impl;
}

int SDMSource::getPropertyAPI(const char *name,char *buf,std::size_t n) {
	return impl().getPropertyAPI(name,buf,n);
}

int SDMSource::setPropertyAPI(const char *name,const char *value) {
	return impl().setPropertyAPI(name,value);
}

void *SDMSource::handle() const {
	return impl().handle();
}

SDMPlugin &SDMSource::plugin() {
	return impl().plugin();
}

const SDMPlugin &SDMSource::plugin() const {
	return impl().plugin();
}

SDMDevice &SDMSource::device() {
	return impl().device();
}

const SDMDevice &SDMSource::device() const {
	return impl().device();
}

void SDMSource::open(const SDMDevice &d,int ch) {
	_impl=std::make_shared<SDMSourceImpl>(d,ch);
}

void SDMSource::close() {
	_impl.reset();
}

void SDMSource::selectReadStreams(const std::vector<int> &streams,std::size_t packets,int df) {
	impl().selectReadStreams(streams,packets,df);
}

int SDMSource::readStream(int stream,sdm_sample_t *data,std::size_t n,Flags flags) {
	return impl().readStream(stream,data,n,flags);
}

void SDMSource::readNextPacket() {
	return impl().readNextPacket();
}

void SDMSource::discardPackets() {
	impl().discardPackets();
}

int SDMSource::readStreamErrors() {
	return impl().readStreamErrors();
}

SDMSource::operator bool() const {
	return _impl.operator bool();
}

int SDMSource::id() const {
	if(!_impl) return -1;
	return impl().id();
}
