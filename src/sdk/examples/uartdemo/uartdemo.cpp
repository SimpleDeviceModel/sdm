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

#include "uartdemo.h"

#include <vector>

/*
 * UartPlugin instance
 */

SDMAbstractPluginProvider *SDMAbstractPluginProvider::instance() {
	static UartPlugin plugin;
	return &plugin;
}

/*
 * UartPlugin members
 */

UartPlugin::UartPlugin() {
	addConstProperty("Name","UART demo");
	addConstProperty("Vendor","Microproject LLC");
	addListItem("Devices","Arduino Uno");
}

SDMAbstractDeviceProvider *UartPlugin::openDevice(int id) {
	if(id!=0) throw std::runtime_error("No device with such ID");
	return new UartDevice();
}

/*
 * UartDevice members
 */

UartDevice::UartDevice() {
	addConstProperty("Name","Arduino Uno");
	addConstProperty("AutoOpenChannels","open");
	addProperty("SerialPort","/dev/ttyACM0");
	addListItem("ConnectionParameters","SerialPort");
	addListItem("Channels","Virtual registers");
}

int UartDevice::close() {
	delete this;
	return 0;
}

SDMAbstractChannelProvider *UartDevice::openChannel(int id) {
	if(id!=0) throw std::runtime_error("No channel with such ID");
	return new UartChannel(_port);
}

SDMAbstractSourceProvider *UartDevice::openSource(int id) {
	return nullptr;
}

int UartDevice::connect() {
	_port.open(getProperty("SerialPort"));
	_port.setBaudRate(115200);
	_port.setDataBits(8);
	_port.setStopBits(Uart::OneStop);
	_port.setParity(Uart::NoParity);
	_port.setFlowControl(Uart::NoFlowControl);
	return 0;
}

int UartDevice::disconnect() {
	_port.close();
	return 0;
}

int UartDevice::getConnectionStatus() {
	if(_port) return 1;
	return 0;
}

/*
 * UartChannel members
 */

UartChannel::UartChannel(Uart &port): _port(port) {
	addConstProperty("Name","Virtual registers");
}

int UartChannel::close() {
	delete this;
	return 0;
}

int UartChannel::writeReg(sdm_addr_t addr,sdm_reg_t data) {
	std::string packet;
	packet.push_back('\x50');
	packet.push_back(static_cast<char>(addr&0xFF));
	packet.push_back(static_cast<char>(data&0xFF));
	sendBytes(packet);
	return 0;
}

sdm_reg_t UartChannel::readReg(sdm_addr_t addr,int *status) {
	std::string packet;
	packet.push_back('\x51');
	packet.push_back(static_cast<char>(addr&0xFF));
	sendBytes(packet);
	std::string r=receiveBytes(2);
	if(status) *status=0;
	return ((r[0]&0xF)<<4)|(r[1]&0xF);
}

void UartChannel::sendBytes(const std::string &s) {
	const char *p=s.data();
	std::size_t towrite=s.size();
	while(towrite>0) {
		std::size_t r=_port.write(p,towrite);
		p+=r;
		towrite-=r;
	}
}

std::string UartChannel::receiveBytes(std::size_t n) {
	std::vector<char> buf(n);
	char *p=buf.data();
	while(n>0) {
		std::size_t r=_port.read(p,n);
		p+=r;
		n-=r;
	}
	return std::string(buf.data(),buf.size());
}

/*
 * UartSource members
 */
/*
sdm_sample_t UartSource::Streams::word(int stream,int i) {
	if(i==0) return static_cast<sdm_sample_t>(npacket[stream]%16384);
	if(i==1) return static_cast<sdm_sample_t>(stream);
	if(i<400) return 0;
	if(i<1000) return static_cast<sdm_sample_t>(stream?(6400-i):i);
	if(i<2000) return (npacket[stream]+(stream?(6400-(i-1000)):(i-1000)))%16384;
	return std::rand()%16384;
}

UartSource::UartSource(int id,const bool &connected):
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

int UartSource::close() {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")!="Quiet")
		std::cout<<"UartPlugin: entered sdmCloseSource()"<<std::endl;

	delete this;
	return 0;
}

int UartSource::selectReadStreams(const int *streams,std::size_t n,std::size_t packets,int df) {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")!="Quiet") {
		std::cout<<"UartPlugin: entered sdmSelectReadStreams()"<<std::endl;
		std::cout<<"UartPlugin: selected streams: ";
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

int UartSource::readStream(int stream,sdm_sample_t *data,std::size_t n,int nb) {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")=="Verbose") {
		std::cout<<"UartPlugin: entered sdmReadStream()"<<std::endl;
		std::cout<<"UartPlugin: "<<n<<" data words requested for stream "<<stream<<std::endl;
		std::cout<<"UartPlugin: "<<(nb?"non-blocking mode":"blocking mode")<<std::endl;
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

int UartSource::readNextPacket() {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")=="Verbose")
		std::cout<<"UartPlugin: entered sdmReadNextPacket()"<<std::endl;
	
	if(!_connected) return SDM_ERROR;
	
	for(int i=0;i<2;i++) {
		_s.npacket[i]+=_s.df;
		_s.pos[i]=0;
	}
	return 0;
}

void UartSource::discardPackets() {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")=="Verbose")
		std::cout<<"UartPlugin: entered sdmDiscardPackets()"<<std::endl;
	
	_s.begin=std::chrono::steady_clock::now();
	for(int i=0;i<2;i++) {
		_s.npacket[i]=0;
		_s.pos[i]=0;
	}
}

int UartSource::readStreamErrors() {
	if(SDMAbstractPluginProvider::instance()->getProperty("Verbosity")=="Verbose")
		std::cout<<"UartPlugin: entered sdmSelectReadStreams()"<<std::endl;	
	
	return 0;
}

void UartSource::setProperty(const std::string &name,const std::string &value) {
	SDMAbstractSourceProvider::setProperty(name,value);
	
	try {
		_msPerPacket=std::max(std::stoi(getProperty("MsPerPacket")),1);
	}
	catch(std::exception &) {
		std::cout<<"Warning: bad property value \""<<value<<"\""<<std::endl;
	}
}
*/