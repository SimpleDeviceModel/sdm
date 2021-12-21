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
 * SDMPlug is a C++ library intended to be used by host applications
 * to access SDM plugins.
 *
 * Each of the main classes (SDMPlugin, SDMDevice etc.) holds
 * a plugin-related resource. To prevent premature resource
 * destruction, copies of instances of these classes are
 * reference-counted. Since some classes depend on the resources
 * of other classes, inheritance hierarchy has been introduced.
 */

#ifndef SDMPLUG_H_INCLUDED
#define SDMPLUG_H_INCLUDED

#include "sdmtypes.h"
#include "safeflags.h"

#include <string>
#include <vector>
#include <memory>

class SDMPluginImpl;
class SDMDeviceImpl;
class SDMChannelImpl;
class SDMSourceImpl;

// Auxiliary classes

struct SDMImport {
	PtrSdmGetPluginProperty ptrGetPluginProperty;
	PtrSdmSetPluginProperty ptrSetPluginProperty;
	
	PtrSdmOpenDevice ptrOpenDevice;
	PtrSdmCloseDevice ptrCloseDevice;
	PtrSdmGetDeviceProperty ptrGetDeviceProperty;
	PtrSdmSetDeviceProperty ptrSetDeviceProperty;
	PtrSdmConnect ptrConnect;
	PtrSdmDisconnect ptrDisconnect;
	PtrSdmGetConnectionStatus ptrGetConnectionStatus;
	
	PtrSdmOpenChannel ptrOpenChannel;
	PtrSdmCloseChannel ptrCloseChannel;
	PtrSdmGetChannelProperty ptrGetChannelProperty;
	PtrSdmSetChannelProperty ptrSetChannelProperty;
	PtrSdmWriteReg ptrWriteReg;
	PtrSdmReadReg ptrReadReg;
	PtrSdmWriteFIFO ptrWriteFIFO;
	PtrSdmReadFIFO ptrReadFIFO;
	PtrSdmWriteMem ptrWriteMem;
	PtrSdmReadMem ptrReadMem;
	
	PtrSdmOpenSource ptrOpenSource;
	PtrSdmCloseSource ptrCloseSource;
	PtrSdmGetSourceProperty ptrGetSourceProperty;
	PtrSdmSetSourceProperty ptrSetSourceProperty;
	PtrSdmSelectReadStreams ptrSelectReadStreams;
	PtrSdmReadStream ptrReadStream;
	PtrSdmReadNextPacket ptrReadNextPacket;
	PtrSdmDiscardPackets ptrDiscardPackets;
	PtrSdmReadStreamErrors ptrReadStreamErrors;
	
	bool supportChannels;
	bool supportSources;
};

class sdmplugin_error : public std::exception {
	std::string message;
	int code=0;
public:
	explicit sdmplugin_error(): message("SDM plugin returned an error") {}
	explicit sdmplugin_error(const std::string &strFuncName);
	explicit sdmplugin_error(const std::string &strFuncName, int iErrorCode);
	virtual ~sdmplugin_error() throw() {}
	
	virtual const char *what() const throw() {return message.c_str();}
	int errorCode() const {return code;}
};

class SDMBase {
protected:
	virtual int getPropertyAPI(const char *name,char *buf,std::size_t n)=0;
	virtual int setPropertyAPI(const char *name,const char *value)=0;
public:
	std::string getProperty(const std::string &name);
	std::string getProperty(const std::string &name,const std::string &defaultValue);
	std::vector<std::string> listProperties(const std::string &name);
	void setProperty(const std::string &name,const std::string &value);
};

// Main class to work with an SDM plugin

class SDMPlugin : virtual public SDMBase {
	std::vector<std::string> _pluginSearchPath;
	std::shared_ptr<SDMPluginImpl> _impl;
	
	SDMPluginImpl &impl();
	const SDMPluginImpl &impl() const;

	virtual int getPropertyAPI(const char *name,char *buf,std::size_t n) override;
	virtual int setPropertyAPI(const char *name,const char *value) override;
	
public:
	SDMPlugin() {}
	explicit SDMPlugin(const std::string &strFileName);
	virtual ~SDMPlugin() {}
	
	void addSearchPath(const std::string &str) {_pluginSearchPath.push_back(str);}

	virtual void open(const std::string &strFileName);
	virtual void close();
	
	const SDMImport &functions() const;
	
	operator bool() const;
	std::string path() const;
};

// Device class

class SDMDevice : virtual public SDMBase {
	std::shared_ptr<SDMDeviceImpl> _impl;
	
	SDMDeviceImpl &impl();
	const SDMDeviceImpl &impl() const;
	
	virtual int getPropertyAPI(const char *name,char *buf,std::size_t n) override;
	virtual int setPropertyAPI(const char *name,const char *value) override;
public:
	SDMDevice() {}
	SDMDevice(const SDMPlugin &pl,int iDev);
	virtual ~SDMDevice() {}
	
	void *handle() const;
	SDMPlugin &plugin();
	const SDMPlugin &plugin() const;
	
	virtual void open(const SDMPlugin &pl,int iDev);
	virtual void close();
	
	virtual void connect();
	virtual void disconnect();
	virtual bool isConnected();
	
	operator bool() const;
	int id() const;
};

// Control channel class

class SDMChannel : virtual public SDMBase {
public:
	typedef SafeFlags<SDMChannel> Flags;
	
	static const Flags Normal;
	static const Flags NonBlocking;
	static const Flags AllowPartial;
	static const Flags StartOfPacket;
	static const Flags NextPacket;
	
	static const int WouldBlock;
private:
	std::shared_ptr<SDMChannelImpl> _impl;
	
	SDMChannelImpl &impl();
	const SDMChannelImpl &impl() const;
	
	virtual int getPropertyAPI(const char *name,char *buf,std::size_t n) override;
	virtual int setPropertyAPI(const char *name,const char *value) override;
public:
	SDMChannel() {}
	SDMChannel(const SDMDevice &d,int ch);
	virtual ~SDMChannel() {}
	
	void *handle() const;
	SDMPlugin &plugin();
	const SDMPlugin &plugin() const;
	SDMDevice &device();
	const SDMDevice &device() const;
	
	virtual void open(const SDMDevice &d,int ch);
	virtual void close();
	
	virtual void writeReg(sdm_addr_t addr,sdm_reg_t data);
	virtual sdm_reg_t readReg(sdm_addr_t addr);
	virtual int writeFIFO(sdm_addr_t addr,const sdm_reg_t *data,std::size_t n,Flags flags=Normal);
	virtual int readFIFO(sdm_addr_t addr,sdm_reg_t *data,std::size_t n,Flags flags=Normal);
	virtual void writeMem(sdm_addr_t addr,const sdm_reg_t *data,std::size_t n);
	virtual void readMem(sdm_addr_t addr,sdm_reg_t *data,std::size_t n);
	
	operator bool() const;
	int id() const;
};

// Data source class

class SDMSource : virtual public SDMBase {
public:
	typedef SafeFlags<SDMSource> Flags;
	
	static const Flags Normal;
	static const Flags NonBlocking;
	static const Flags AllowPartial;
	
	static const int WouldBlock;
private:
	std::shared_ptr<SDMSourceImpl> _impl;
	
	SDMSourceImpl &impl();
	const SDMSourceImpl &impl() const;
	
	virtual int getPropertyAPI(const char *name,char *buf,std::size_t n) override;
	virtual int setPropertyAPI(const char *name,const char *value) override;
public:
	SDMSource() {}
	SDMSource(const SDMDevice &d,int src);
	virtual ~SDMSource() {}
	
	void *handle() const;
	SDMPlugin &plugin();
	const SDMPlugin &plugin() const;
	SDMDevice &device();
	const SDMDevice &device() const;
	
	virtual void open(const SDMDevice &d,int src);
	virtual void close();
	
	virtual void selectReadStreams(const std::vector<int> &streams,std::size_t packets,int df);
	virtual int readStream(int stream,sdm_sample_t *data,std::size_t n,Flags flags=Normal);
	virtual void readNextPacket();
	virtual void discardPackets();
	virtual int readStreamErrors();
	
	operator bool() const;
	int id() const;
};

#endif
