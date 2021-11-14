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
 * This module implements an SDM plugin used in the test suite.
 */

#include "testplugin.h"

#include <iostream>
#include <thread>
#include <cstring>
#include <climits>
#include <cstdlib>
#include <exception>
#include <algorithm>

/*
 * TestPlugin instance
 */

SDMAbstractPluginProvider *SDMAbstractPluginProvider::instance() {
	static TestPlugin plugin;
	return &plugin;
}

/*
 * TestPlugin members
 */

TestPlugin::TestPlugin() {
	addConstProperty("Name","Software simulated test");
	addConstProperty("Vendor","Microproject LLC");
	
	addListItem("Devices","Test device 1");
	addListItem("Devices","Test device 2");
	
	addProperty("Verbosity","Default");
	addProperty("AutoOpenMode","open");
	addProperty("DisableChildProperties","false");
}

SDMAbstractDeviceProvider *TestPlugin::openDevice(int id) {
	if(getProperty("Verbosity")!="Quiet")
		std::cout<<"testplugin: entered sdmOpenDevice()"<<std::endl;

	if(id!=0&&id!=1) return nullptr;
	try {
		return new TestDevice(id);
	}
	catch(std::exception &) {
		return nullptr;
	}
}

/*
 * TestDevice members
 */

TestDevice::TestDevice(int id) {
	if(SDMAbstractPluginProvider::instance()->getProperty("DisableChildProperties")=="true") return;
	addConstProperty("Name","Test device "+std::to_string(id+1));
	
	auto const &aom=SDMAbstractPluginProvider::instance()->getProperty("AutoOpenMode");
	addConstProperty("AutoOpenChannels",aom);
	addConstProperty("AutoOpenSources",aom);
	
	addProperty("IPaddress","A.B.C.D");
	addListItem("ConnectionParameters","IPaddress");
	addProperty("Port","XXXX");
	addListItem("ConnectionParameters","Port");
	
	addListItem("Channels","Measurement equipment");
	addListItem("Channels","Unit under test");
	addListItem("Sources","Source 1");
	addListItem("Sources","Source 2");
	addListItem("Sources","Video source");
}

int TestDevice::close() {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")!="Quiet")
		std::cout<<"testplugin: entered sdmCloseDevice()"<<std::endl;
	delete this;
	return 0;
}

SDMAbstractChannelProvider *TestDevice::openChannel(int id) {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")!="Quiet")
		std::cout<<"testplugin: entered sdmOpenChannel()"<<std::endl;
	
	if(id!=0&&id!=1) return nullptr;
	try {
		return new TestChannel(id,_connected);
	}
	catch(std::exception &) {
		return nullptr;
	}
}

SDMAbstractSourceProvider *TestDevice::openSource(int id) {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")!="Quiet")
		std::cout<<"testplugin: entered sdmOpenSource()"<<std::endl;
	
	try {
		if(id==0||id==1) {
			return new TestSource(id,_connected);
		}
		else if(id==2) {
			return new VideoSource(_connected);
		}
		else return nullptr;
	}
	catch(std::exception &) {
		return nullptr;
	}
}

int TestDevice::connect() {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")!="Quiet") {
		std::cout<<"testplugin: entered sdmConnect()"<<std::endl;
		if(SDMAbstractPluginProvider::instance()->getProperty("DisableChildProperties")!="true") {
			std::cout<<"testplugin: connection parameters are: "<<
				"IPaddress=\""<<getProperty("IPaddress")<<"\", "<<
				"Port=\""<<getProperty("Port")<<"\""<<std::endl;
		}
	}

	_connected=true;
	return 0;
}

int TestDevice::disconnect() {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")!="Quiet")
		std::cout<<"testplugin: entered sdmDisconnect()"<<std::endl;

	_connected=false;
	return 0;
}

int TestDevice::getConnectionStatus() {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")=="Verbose")
		std::cout<<"testplugin: entered sdmGetConnectionStatus()"<<std::endl;

	if(_connected) return 1;
	return 0;
}

/*
 * TestChannel members
 */

TestChannel::TestChannel(int id,const bool &connected):
	_id(id),
	_connected(connected)
{
	if(SDMAbstractPluginProvider::instance()->getProperty("DisableChildProperties")=="true") return;
	if(_id==0) {
		addConstProperty("Name","Measurement equipment");
	}
	else if(_id==1) {
		addConstProperty("Name","Unit under test");
	}
}

int TestChannel::close() {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")!="Quiet")
		std::cout<<"testplugin: entered sdmCloseChannel()"<<std::endl;

	delete this;
	return 0;
}

int TestChannel::writeReg(sdm_addr_t addr,sdm_reg_t data) {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")=="Verbose") {
		std::cout<<"testplugin: entered sdmWriteReg()"<<std::endl;
		std::cout<<"testplugin: writing "<<data<<" to address "<<addr<<std::endl;
	}
	
	if(!_connected) return SDM_ERROR;

	if(addr>=256) return SDM_ERROR;
	
	if(addr!=0) { // plain register
		_regs[addr]=data;
	}
	else { // dedicated FIFO at address 0
		_fifo0.emplace_back(data,false);
	}
	
	return 0;
}

sdm_reg_t TestChannel::readReg(sdm_addr_t addr,int *status) {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")=="Verbose") {
		std::cout<<"testplugin: entered sdmReadReg()"<<std::endl;
		std::cout<<"testplugin: requested read from address "<<addr<<std::endl;
	}

	if(!_connected||addr>=256) {
		if(status) *status=-1;
		return 0;
	}
	if(status) *status=0;
	
	sdm_reg_t data;
	
	if(addr!=0) { // plain register
		if(addr==127) {
			std::cout<<"testplugin: emulating long operation: waiting 10 sec"<<std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(10));
		}
		data=_regs[addr];
	}
	else { // dedicated FIFO at address 0
		if(!_fifo0.empty()) {
			data=_fifo0.front().word;
			_fifo0.pop_front();
		}
		else {
			std::cout<<"dummmyplugin: warning: FIFO is empty"<<std::endl;
			data=0;
		}
	}
	
	return data;
}

int TestChannel::writeFIFO(sdm_addr_t addr,const sdm_reg_t *data,std::size_t n,int flags) {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")=="Verbose") {
		std::cout<<"testplugin: entered sdmWriteFIFO()"<<std::endl;
		if(flags&SDM_FLAG_NONBLOCKING) std::cout<<"testplugin: a non-blocking operation requested"<<std::endl;
		if(flags&SDM_FLAG_START) std::cout<<"testplugin: start of packet"<<std::endl;
		std::cout<<"testplugin: writing "<<n<<" values to the address "<<addr<<std::endl;
		std::cout<<"testplugin: content: ";
		for(std::size_t i=0;i<n;i++) std::cout<<data[i]<<" ";
		std::cout<<std::endl;
	}
	
	if(!_connected) return SDM_ERROR;
	if(n>INT_MAX) n=INT_MAX;
	
	if(addr!=0) { // plain register, no need to write all values
		if(addr>=256) return SDM_ERROR;
		_regs[addr]=data[n-1];
	}
	else { // dedicated FIFO at address 0
		for(std::size_t i=0;i<n;i++) {
			bool start=(flags&SDM_FLAG_START)&&(i==0);
			_fifo0.emplace_back(data[i],start);
		}
	}
	
	return static_cast<int>(n);
}

int TestChannel::readFIFO(sdm_addr_t addr,sdm_reg_t *data,std::size_t n,int flags) {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")=="Verbose") {
		std::cout<<"testplugin: entered sdmReadFIFO()"<<std::endl;
		if(flags&SDM_FLAG_NONBLOCKING) std::cout<<"testplugin: a non-blocking operation requested"<<std::endl;
		if(flags&SDM_FLAG_NEXT) std::cout<<"testplugin: next packet requested"<<std::endl;
		std::cout<<"testplugin: requested "<<n<<" values from address "<<addr<<std::endl;
	}
	
	if(!_connected) return SDM_ERROR;
	if(n>INT_MAX) n=INT_MAX;
	
	if(flags&SDM_FLAG_NEXT) { // skip data until the next packet, or until fifo is empty
		if(addr==0) {
			while(!_fifo0.empty()) {
				if(_fifo0.front().sop) {
					_fifo0_next=true;
					break;
				}
				_fifo0.pop_front();
			}
		}
	}
	
	std::size_t i;
	if(addr!=0) { // plain register
		if(addr>=256) return SDM_ERROR;
		for(i=0;i<n;i++) {
			data[i]=_regs[addr];
		}
	}
	else { // dedicated FIFO at address 0
		for(i=0;i<n;i++) {
			if(_fifo0.empty()) {
				if(flags&SDM_FLAG_NONBLOCKING) {
					if(i==0) return SDM_WOULDBLOCK;
					else break;
				}
// Blocking operations allow partial read as long as at least 1 word is available
				else if(i>0) break;
				else {
					std::cout<<"dummmyplugin: FIFO is empty: deadlock!"<<std::endl;
					return SDM_ERROR;
				}
			}
			if(_fifo0.front().sop) { // new packet
				if(i>0||!_fifo0_next) {
					return static_cast<int>(i);
				}
				else _fifo0_next=false;
			}
			data[i]=_fifo0.front().word;
			_fifo0.pop_front();
		}
	}
	
	return static_cast<int>(i);
}

int TestChannel::writeMem(sdm_addr_t addr,const sdm_reg_t *data,std::size_t n) {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")=="Verbose") {
		std::cout<<"testplugin: entered sdmWriteMem()"<<std::endl;
		std::cout<<"testplugin: writing "<<n<<" words to address "<<addr<<std::endl;
		std::cout<<"testplugin: content: [";
		for(std::size_t i=0;i<n;i++) {
			if(i>0) std::cout<<' ';
			std::cout<<data[i];
		}
		std::cout<<"]"<<std::endl;
	}
	
	if(!_connected) return SDM_ERROR;
	
	if(n==0) return 0;

	if(addr+n>256) return SDM_ERROR;
	std::memcpy(&_regs[addr],data,n*sizeof(sdm_reg_t));
	return 0;
}

int TestChannel::readMem(sdm_addr_t addr,sdm_reg_t *data,std::size_t n) {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")=="Verbose") {
		std::cout<<"testplugin: entered sdmReadMem()"<<std::endl;
		std::cout<<"testplugin: requested "<<n<<" words from address "<<addr<<std::endl;
	}
	
	if(!_connected) return SDM_ERROR;

	if(addr+n>256) return SDM_ERROR;
	std::memcpy(data,&_regs[addr],n*sizeof(sdm_reg_t));
	return 0;
}

/*
 * TestSource members
 */

sdm_sample_t TestSource::Streams::word(int stream,int i) {
	if(i==0) return static_cast<sdm_sample_t>(npacket[stream]%16384);
	if(i==1) return static_cast<sdm_sample_t>(stream);
	if(i<400) return 0;
	if(i<1000) return static_cast<sdm_sample_t>(stream?(6400-i):i);
	if(i<2000) return (npacket[stream]+(stream?(6400-(i-1000)):(i-1000)))%16384;
	return std::rand()%16384;
}

TestSource::TestSource(int id,const bool &connected):
	_id(id),
	_connected(connected)
{
	if(SDMAbstractPluginProvider::instance()->getProperty("DisableChildProperties")=="true") return;
	if(_id==0) addConstProperty("Name","Source 1");
	else if(_id==1) addConstProperty("Name","Source 2");
	if(_id==0) addConstProperty("ShowStreams","0,1");
	else if(_id==1) addConstProperty("ShowStreams","0,1");
	addProperty("MsPerPacket",std::to_string(_msPerPacket));
	addListItem("Streams","Stream 1");
	addListItem("Streams","Stream 2");
	addListItem("UserScripts","Signal Analyzer");
	addListItem("UserScripts","signal_analyzer.lua");
}

int TestSource::close() {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")!="Quiet")
		std::cout<<"testplugin: entered sdmCloseSource()"<<std::endl;

	delete this;
	return 0;
}

int TestSource::selectReadStreams(const int *streams,std::size_t n,std::size_t packets,int df) {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")!="Quiet") {
		std::cout<<"testplugin: entered sdmSelectReadStreams()"<<std::endl;
		std::cout<<"testplugin: selected streams: ";
		for(std::size_t i=0;i<n;i++) std::cout<<streams[i]<<", ";
		std::cout<<"suggested number of packets: "<<packets<<", decimation factor: "<<df<<std::endl;
	}
	
	if(!_connected) return SDM_ERROR;
	
	for(std::size_t i=0;i<n;i++) if(streams[i]!=0&&streams[i]!=1) return SDM_ERROR;
	
	for(std::size_t i=0;i<2;i++) _s.selectedStreams[i]=false;
	
	if(n==0) return 0;
	
	for(std::size_t i=0;i<n;i++) {
		_s.selectedStreams[streams[i]]=true;
		_s.npacket[streams[i]]=0;
	}
	
	_s.begin=std::chrono::steady_clock::now();
	
	if(df<1) return SDM_ERROR;
	_s.df=df;
	return 0;
}

int TestSource::readStream(int stream,sdm_sample_t *data,std::size_t n,int nb) {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")=="Verbose") {
		std::cout<<"testplugin: entered sdmReadStream()"<<std::endl;
		std::cout<<"testplugin: "<<n<<" data words requested for stream "<<stream<<std::endl;
		std::cout<<"testplugin: "<<(nb?"non-blocking mode":"blocking mode")<<std::endl;
	}
	
	if(n==0) return 0;
	if(n>INT_MAX) n=INT_MAX;
	
	if(!_connected) return SDM_ERROR;
	
	if(stream<0||stream>1) return SDM_ERROR;
	
	if(!_s.selectedStreams[stream]) return SDM_ERROR; // reading of non-selected streams is not allowed
	
	const int expectedPacket=_s.npacket[stream];
	
	for(;;) {
		int &pos=_s.pos[stream];
		if(pos==6400) return 0; // end of packet
		
		const int timeElapsed=static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>
			(std::chrono::steady_clock::now()-_s.begin).count());
		
		const int currentPacketTime=timeElapsed-expectedPacket*_msPerPacket;
		int availableSamples=0;
		
		if(currentPacketTime>0) {
			if(currentPacketTime>=_msPerPacket) availableSamples=6400; // full packet
			else availableSamples=currentPacketTime*6400/_msPerPacket; // partial packet
		}
		
		if(availableSamples==0||pos>=availableSamples) {
// Data are not available yet. Return if nonblocking, wait otherwise
			if(nb) return SDM_WOULDBLOCK;
			if(currentPacketTime<0)
				std::this_thread::sleep_for(std::chrono::milliseconds(-currentPacketTime));
			else std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}
		
		int toread;
		if(static_cast<int>(n)<availableSamples-pos) toread=static_cast<int>(n);
		else toread=availableSamples-pos;
		
// Read data
		for(int i=0;i<toread;i++) data[i]=_s.word(stream,i+pos);
		pos+=toread;
		return toread;
	}
}

int TestSource::readNextPacket() {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")=="Verbose")
		std::cout<<"testplugin: entered sdmReadNextPacket()"<<std::endl;
	
	if(!_connected) return SDM_ERROR;
	
	for(int i=0;i<2;i++) {
		_s.npacket[i]+=_s.df;
		_s.pos[i]=0;
	}
	return 0;
}

void TestSource::discardPackets() {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")=="Verbose")
		std::cout<<"testplugin: entered sdmDiscardPackets()"<<std::endl;
	
	_s.begin=std::chrono::steady_clock::now();
	for(int i=0;i<2;i++) {
		_s.npacket[i]=0;
		_s.pos[i]=0;
	}
}

int TestSource::readStreamErrors() {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")=="Verbose")
		std::cout<<"testplugin: entered sdmSelectReadStreams()"<<std::endl;	
	
	return 0;
}

void TestSource::setProperty(const std::string &name,const std::string &value) {
	SDMAbstractSourceProvider::setProperty(name,value);
	
	try {
		_msPerPacket=std::max(std::stoi(getProperty("MsPerPacket")),1);
	}
	catch(std::exception &) {
		std::cout<<"Warning: bad property value \""<<value<<"\""<<std::endl;
	}
}


/*
 * VideoSource members
 */

VideoSource::VideoSource(const bool &connected):
	_connected(connected)
{
	addConstProperty("Name","Video source");
	addConstProperty("ViewMode","video");
	addProperty("MsPerPacket",std::to_string(_msPerPacket));
	addListItem("Streams","Video stream");
	addListItem("UserScripts","Signal Analyzer");
	addListItem("UserScripts","signal_analyzer.lua");

// Video properties
	addProperty("SimpleMode",_simpleMode?"true":"false");
	addProperty("Width",std::to_string(_frame.width()));
	addProperty("Height",std::to_string(_frame.height()));
	addProperty("Radius",std::to_string(_frame.radius()));
	addProperty("Blur",std::to_string(_frame.blur()));
	addProperty("Margin",std::to_string(_frame.margin()));
	addProperty("Black",std::to_string(_frame.black()));
	addProperty("White",std::to_string(_frame.white()));
	addProperty("Noise",std::to_string(_frame.noise()));
	addProperty("Velocity",std::to_string(_frame.velocity()));
	addProperty("Gravity",std::to_string(_frame.gravity()));
	addProperty("OffsetX",std::to_string(_frame.offsetX()));
	addProperty("OffsetY",std::to_string(_frame.offsetY()));
}

int VideoSource::close() {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")!="Quiet")
		std::cout<<"testplugin: entered sdmCloseSource()"<<std::endl;

	delete this;
	return 0;
}

int VideoSource::selectReadStreams(const int *streams,std::size_t n,std::size_t packets,int df) {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")!="Quiet") {
		std::cout<<"testplugin: entered sdmSelectReadStreams()"<<std::endl;
		std::cout<<"testplugin: selected streams: ";
		for(std::size_t i=0;i<n;i++) std::cout<<streams[i]<<", ";
		std::cout<<"suggested number of packets: "<<packets<<", decimation factor: "<<df<<std::endl;
	}
	
	if(!_connected) return SDM_ERROR;
	
	if(n==0) { // deselect the stream
		_selected=false;
		return 0;
	}
	else if(n==1&&*streams==0) _selected=true;
	else return SDM_ERROR;
	
	if(df<1) return SDM_ERROR;
	_df=df;
	
	_begin=std::chrono::steady_clock::now();
	
	return 0;
}

int VideoSource::readStream(int stream,sdm_sample_t *data,std::size_t n,int nb) {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")=="Verbose") {
		std::cout<<"testplugin: entered sdmReadStream()"<<std::endl;
		std::cout<<"testplugin: "<<n<<" data words requested for stream "<<stream<<std::endl;
		std::cout<<"testplugin: "<<(nb?"non-blocking mode":"blocking mode")<<std::endl;
	}
	
	if(n==0) return 0;
	if(n>INT_MAX) n=INT_MAX;
	
	if(!_connected) return SDM_ERROR;
	if(stream!=0) return SDM_ERROR;
	
	auto size=_frame.width()*_frame.height();
	
	if(!_selected) {
// Asynchronous read
		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // a small cool-down delay
		int toread=(static_cast<int>(n)<size)?static_cast<int>(n):size;
		if(!_simpleMode) {
			for(int i=0;i<toread;i++) {
				auto x=i%_frame.width();
				auto y=i/_frame.width();
				data[i]=_frame.sample(x,y);
			}
		}
		else {
			for(int i=0;i<toread;i++) data[i]=(_npacket+i)%16384;
		}
		_npacket++;
		return toread;
	}
	else {
// Synchronous read
		const int expectedPacket=_npacket;
		
		for(;;) {
			if(_pos==size) return 0; // end of packet
			
			const int timeElapsed=static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>
				(std::chrono::steady_clock::now()-_begin).count());
			
			const int currentPacketTime=timeElapsed-expectedPacket*_msPerPacket;
			int availableSamples=0;
			
			if(currentPacketTime>_msPerPacket) availableSamples=size; // full packet
			
			if(availableSamples==0||_pos>=availableSamples) {
// Data are not available yet. Return if nonblocking, wait otherwise
				if(nb) return SDM_WOULDBLOCK;
				if(currentPacketTime<0)
					std::this_thread::sleep_for(std::chrono::milliseconds(-currentPacketTime));
				else std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}
			
			int toread;
			if(static_cast<int>(n)<availableSamples-_pos) toread=static_cast<int>(n);
			else toread=availableSamples-_pos;
			
// Read data
			if(!_simpleMode) {
				for(int i=0;i<toread;i++) {
					auto x=(i+_pos)%_frame.width();
					auto y=(i+_pos)/_frame.width();
					data[i]=_frame.sample(x,y);
				}
			}
			else {
// Simple mode is designed for performance testing and
// is generated very fast (does not create bottleneck)
				for(int i=0;i<toread;i++) data[i]=(_npacket+i+_pos)%16384;
			}
			_pos+=toread;
			return toread;
		}
	}
}

int VideoSource::readNextPacket() {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")=="Verbose")
		std::cout<<"testplugin: entered sdmReadNextPacket()"<<std::endl;
	
	if(!_connected) return SDM_ERROR;
	
	_npacket+=_df;
	_pos=0;
	
	if(!_simpleMode) _frame.next();
	
	return 0;
}

void VideoSource::discardPackets() {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")=="Verbose")
		std::cout<<"testplugin: entered sdmDiscardPackets()"<<std::endl;
	
	_begin=std::chrono::steady_clock::now();
	_npacket=0;
	_pos=0;
}

int VideoSource::readStreamErrors() {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")=="Verbose")
		std::cout<<"testplugin: entered sdmSelectReadStreams()"<<std::endl;	
	
	return 0;
}

void VideoSource::setProperty(const std::string &name,const std::string &value) {
	SDMAbstractSourceProvider::setProperty(name,value);
	
	try {
		_msPerPacket=std::max(std::stoi(getProperty("MsPerPacket")),1);
		_simpleMode=(getProperty("SimpleMode")=="true");
		_frame.setWidth(std::stoi(getProperty("Width")));
		_frame.setHeight(std::stoi(getProperty("Height")));
		_frame.setRadius(std::stod(getProperty("Radius")));
		_frame.setBlur(std::stod(getProperty("Blur")));
		_frame.setMargin(std::stod(getProperty("Margin")));
		_frame.setBlack(std::stod(getProperty("Black")));
		_frame.setWhite(std::stod(getProperty("White")));
		_frame.setNoise(std::stod(getProperty("Noise")));
		_frame.setVelocity(std::stod(getProperty("Velocity")));
		_frame.setGravity(std::stod(getProperty("Gravity")));
		_frame.setOffsetX(std::stod(getProperty("OffsetX")));
		_frame.setOffsetY(std::stod(getProperty("OffsetY")));
	}
	catch(std::exception &) {
		std::cout<<"Warning: bad property value \""<<value<<"\""<<std::endl;
	}
}
