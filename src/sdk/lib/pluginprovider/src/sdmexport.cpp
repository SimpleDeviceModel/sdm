/*
 * Copyright (c) 2015-2021 by Microproject LLC
 * 
 * This file is part of the Simple Device Model (SDM) framework SDK.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * This file provides exported SDM functions for use by plugins
 * based on pluginprovider library. The object file produced from this
 * module should be linked with the plugin shared library.
 */

#define EXPORT_SDM_SYMBOLS

#include "sdmapi.h"
#include "sdmprovider.h"

#include <iostream>
#include <stdexcept>
#include <cstring>
#include <climits>

namespace {
	void displayErrorMessage(const char *what) {
		std::cout<<"Exception in SDM plugin: "<<what<<std::endl;
	}

	int getProperty(const SDMPropertyManager *obj,const char *name,char *buf,std::size_t n) {
		try {
			std::string value=obj->getProperty(name);
			std::size_t size=value.size()+1; // plus terminating null character
			if(!size||size>INT_MAX) return SDM_ERROR;
			if(n<size) return static_cast<int>(size);
			std::memcpy(buf,value.c_str(),size);
			return 0;
		}
		catch(std::exception &) {
			return SDM_ERROR;
		}
	}

	int setProperty(SDMPropertyManager *obj,const char *name,const char *value) {
		try {
			obj->setProperty(name,value);
			return 0;
		}
		catch(std::exception &) {
			return SDM_ERROR;
		}

	}
}

/********************************************************************
 * Plugin functions
 *******************************************************************/

SDMAPI int SDMCALL sdmGetPluginProperty(const char *name,char *buf,std::size_t n) {
	try {
		return getProperty(SDMAbstractPluginProvider::instance(),name,buf,n);
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

SDMAPI int SDMCALL sdmSetPluginProperty(const char *name,const char *value) {
	try {
		return setProperty(SDMAbstractPluginProvider::instance(),name,value);
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

/********************************************************************
 * Device functions
 *******************************************************************/

SDMAPI void * SDMCALL sdmOpenDevice(int id) {
	try {
		return SDMAbstractPluginProvider::instance()->openDevice(id);
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return NULL;
	}
}

SDMAPI int SDMCALL sdmCloseDevice(void *h) {
	try {
		return static_cast<SDMAbstractDeviceProvider*>(h)->close();
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

SDMAPI int SDMCALL sdmGetDeviceProperty(void *h,const char *name,char *buf,std::size_t n) {
	try {
		return getProperty(static_cast<SDMAbstractDeviceProvider*>(h),name,buf,n);
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

SDMAPI int SDMCALL sdmSetDeviceProperty(void *h,const char *name,const char *value) {
	try {
		return setProperty(static_cast<SDMAbstractDeviceProvider*>(h),name,value);
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

SDMAPI int SDMCALL sdmConnect(void *h) {
	try {
		return static_cast<SDMAbstractDeviceProvider*>(h)->connect();
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

SDMAPI int SDMCALL sdmDisconnect(void *h) {
	try {
		return static_cast<SDMAbstractDeviceProvider*>(h)->disconnect();
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

SDMAPI int SDMCALL sdmGetConnectionStatus(void *h) {
	try {
		return static_cast<SDMAbstractDeviceProvider*>(h)->getConnectionStatus();
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

/********************************************************************
 * Control channel functions
 *******************************************************************/

SDMAPI void * SDMCALL sdmOpenChannel(void *hdev,int id) {
	try {
		return static_cast<SDMAbstractDeviceProvider*>(hdev)->openChannel(id);
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return NULL;
	}
}

SDMAPI int SDMCALL sdmCloseChannel(void *h) {
	try {
		return static_cast<SDMAbstractChannelProvider*>(h)->close();
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

SDMAPI int SDMCALL sdmGetChannelProperty(void *h,const char *name,char *buf,std::size_t n) {
	try {
		return getProperty(static_cast<SDMAbstractChannelProvider*>(h),name,buf,n);
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

SDMAPI int SDMCALL sdmSetChannelProperty(void *h,const char *name,const char *value) {
	try {
		return setProperty(static_cast<SDMAbstractChannelProvider*>(h),name,value);
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

SDMAPI int SDMCALL sdmWriteReg(void *h,sdm_addr_t addr,sdm_reg_t data) {
	try {
		return static_cast<SDMAbstractChannelProvider*>(h)->writeReg(addr,data);
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

SDMAPI sdm_reg_t SDMCALL sdmReadReg(void *h,sdm_addr_t addr,int *status) {
	try {
		return static_cast<SDMAbstractChannelProvider*>(h)->readReg(addr,status);
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		if(status) *status=-1;
		return 0;
	}
}

SDMAPI int SDMCALL sdmWriteFIFO(void *h,sdm_addr_t addr,const sdm_reg_t *data,std::size_t n,int flags) {
	try {
		return static_cast<SDMAbstractChannelProvider*>(h)->writeFIFO(addr,data,n,flags);
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

SDMAPI int SDMCALL sdmReadFIFO(void *h,sdm_addr_t addr,sdm_reg_t *data,std::size_t n,int flags) {
	try {
		return static_cast<SDMAbstractChannelProvider*>(h)->readFIFO(addr,data,n,flags);
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

SDMAPI int SDMCALL sdmWriteMem(void *h,sdm_addr_t addr,const sdm_reg_t *data,std::size_t n) {
	try {
		return static_cast<SDMAbstractChannelProvider*>(h)->writeMem(addr,data,n);
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

SDMAPI int SDMCALL sdmReadMem(void *h,sdm_addr_t addr,sdm_reg_t *data,std::size_t n) {
	try {
		return static_cast<SDMAbstractChannelProvider*>(h)->readMem(addr,data,n);
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

/********************************************************************
 * Data source functions
 *******************************************************************/

SDMAPI void * SDMCALL sdmOpenSource(void *hdev,int id) {
	try {
		return static_cast<SDMAbstractDeviceProvider*>(hdev)->openSource(id);
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return NULL;
	}
}

SDMAPI int SDMCALL sdmCloseSource(void *h) {
	try {
		return static_cast<SDMAbstractSourceProvider*>(h)->close();
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

SDMAPI int SDMCALL sdmGetSourceProperty(void *h,const char *name,char *buf,std::size_t n) {
	try {
		return getProperty(static_cast<SDMAbstractSourceProvider*>(h),name,buf,n);
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

SDMAPI int SDMCALL sdmSetSourceProperty(void *h,const char *name,const char *value) {
	try {
		return setProperty(static_cast<SDMAbstractSourceProvider*>(h),name,value);
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

SDMAPI int SDMCALL sdmSelectReadStreams(void *h,const int *streams,std::size_t n,std::size_t packets,int df) {
	try {
		return static_cast<SDMAbstractSourceProvider*>(h)->selectReadStreams(streams,n,packets,df);
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

SDMAPI int SDMCALL sdmReadStream(void *h,int stream,sdm_sample_t *data,std::size_t n,int nb) {
	try {
		return static_cast<SDMAbstractSourceProvider*>(h)->readStream(stream,data,n,nb);
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

SDMAPI int SDMCALL sdmReadNextPacket(void *h) {
	try {
		return static_cast<SDMAbstractSourceProvider*>(h)->readNextPacket();
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}

SDMAPI void SDMCALL sdmDiscardPackets(void *h) {
	try {
		static_cast<SDMAbstractSourceProvider*>(h)->discardPackets();
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
	}
}

SDMAPI int SDMCALL sdmReadStreamErrors(void *h) {
	try {
		return static_cast<SDMAbstractSourceProvider*>(h)->readStreamErrors();
	}
	catch(std::exception &ex) {
		displayErrorMessage(ex.what());
		return SDM_ERROR;
	}
}
