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
 * This module implements the UartDemo plugin classes.
 */

#include "uartdemo.h"

#include <vector>
#include <thread>
#include <chrono>

#define MAXBUFSIZE 65536
#define DEFAULT_PACKET_SIZE 500

/*
 * UartPlugin instance
 * 
 * This static member function returns a pointer to the UartPlugin instance.
 * The object itself is defined as a static local variable to ensure
 * that it is initialized before use, thus avoiding the so-called "static
 * initialization order fiasco".
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
	addConstProperty("AutoOpenSources","open");
#ifdef _WIN32
	addProperty("SerialPort","COM1");
#else
	addProperty("SerialPort","/dev/ttyACM0");
#endif
	addListItem("ConnectionParameters","SerialPort");
	addListItem("Channels","Digital pins");
	addListItem("Sources","Virtual oscilloscope");
}

int UartDevice::close() {
	delete this;
	return 0;
}

SDMAbstractChannelProvider *UartDevice::openChannel(int id) {
	if(id!=0) throw std::runtime_error("No channel with such ID");
	return new UartChannel(_port,_q);
}

SDMAbstractSourceProvider *UartDevice::openSource(int id) {
	if(id!=0) throw std::runtime_error("No source with such ID");
	return new UartSource(_port,_q);
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

UartChannel::UartChannel(Uart &port,std::deque<char> &q): _port(port),_q(q) {
	addConstProperty("Name","Digital pins");
	addConstProperty("RegisterMapFile","uartdemo/uartdemo.srm");
}

int UartChannel::close() {
	delete this;
	return 0;
}

int UartChannel::writeReg(sdm_addr_t addr,sdm_reg_t data) {
// Send "Write register" command packet: 0x50 ADDR[7:0] DATA[7:0]
	std::string packet;
	packet.push_back('\x50');
	packet.push_back(static_cast<char>(addr&0xFF));
	packet.push_back(static_cast<char>(data&0xFF));
	sendBytes(packet);
	return 0;
}

sdm_reg_t UartChannel::readReg(sdm_addr_t addr,int *status) {
// Send "Read register" command packet: 0x51 ADDR[7:0]
	std::string packet;
	packet.push_back('\x51');
	packet.push_back(static_cast<char>(addr&0xFF));
	sendBytes(packet);
// Receive data
	char ch,ch2;
	for(int i=0;;i++) {
		auto r=_port.read(&ch,1);
		if(r==0) throw std::runtime_error("Can't read data from the serial port");
		if((ch&0xC0)!=0x80) { // not a register data packet, add to queue
			if(_q.size()<MAXBUFSIZE) _q.push_back(ch);
		}
		else break;
		if(i==10000) throw std::runtime_error("Device is not responding");
	}
	auto r=_port.read(&ch2,1);
	if(r==0) throw std::runtime_error("Can't read data from the serial port");
	if(status) *status=0;
	return ((ch&0xF)<<4)|(ch2&0xF);
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

/*
 * UartSource members
 */

UartSource::UartSource(Uart &port,std::deque<char> &q): _port(port),_q(q) {
	addConstProperty("Name","Virtual oscilloscope");
	addListItem("Streams","A0");
	addListItem("UserScripts","Signal Analyzer");
	addListItem("UserScripts","signal_analyzer.lua");
	addProperty("PacketSize",std::to_string(DEFAULT_PACKET_SIZE));
}

int UartSource::close() {
	delete this;
	return 0;
}

int UartSource::selectReadStreams(const int *streams,std::size_t n,std::size_t packets,int df) {
// In this example we don't need to do anything specific when streams
// are selected since the device is always transmitting data.
// Nevertheless, reading stream data before selecting the stream is an error
// per SDM API spec, so we check for that.
// Here we have only one stream, so the user can select either it or nothing.
	if(n==0) _selected=false;
	else if(n==1&&streams[0]==0) _selected=true;
	else throw std::runtime_error("Bad stream set");
	
	UartSource::discardPackets();
	return 0;
}

int UartSource::readStream(int stream,sdm_sample_t *data,std::size_t n,int nb) {
// Reading stream data before selecting the stream is an error
	if(!_selected) throw std::runtime_error("Stream not selected");
// We have only one stream
	if(stream!=0) throw std::runtime_error("Bad stream ID");
// Note: for a faster interface, it would have been preferable to override
// SDMPropertyManager::setProperty() to avoid string -> integer conversion
// each time UartSource::readStream() is called
	std::size_t packetSize;
	try {
		packetSize=std::stoul(getProperty("PacketSize"),nullptr,0);
		if(packetSize==0) packetSize=DEFAULT_PACKET_SIZE;
	}
	catch(std::exception &) {
		packetSize=DEFAULT_PACKET_SIZE;
	}
	if(_cnt>packetSize) _cnt=packetSize;
// After delivering packetSize samples, return 0 until readNextPacket() is called
	if(n>packetSize-_cnt) n=packetSize-_cnt;
	if(n==0) return 0; // end of packet
	
// Process data from the queue
	std::size_t loaded=loadFromQueue(data,n);
	
	do {
// Read new data from the serial port
		if(_q.size()<MAXBUFSIZE) {
			std::size_t toread=MAXBUFSIZE-_q.size();
			bool nonBlocking=(nb!=0||loaded==n); // don't need to block if we already filled the user's buffer
			std::vector<char> buf(toread);
			auto r=_port.read(buf.data(),toread,nonBlocking?0:-1); // will read "toread" bytes or fewer
			_q.insert(_q.end(),buf.begin(),buf.begin()+r);
		}
		
		if(loaded<n) loaded+=loadFromQueue(data+loaded,n-loaded);
	} while(loaded==0&&nb==0);
	
	_cnt+=loaded;
// Note that "loaded" can't be zero in the blocking mode due to the preceding loop
	if(loaded==0) return SDM_WOULDBLOCK;
	return static_cast<int>(loaded);
}

int UartSource::readNextPacket() {
	_cnt=0;
	return 0;
}

void UartSource::discardPackets() {
	_cnt=0;
	_q.clear();
// Note: after the first readAll() there may still be some out-of-sequence
// data in the serial port buffer. Wait a bit and repeat.
	_port.readAll();
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	_port.readAll();
}

int UartSource::readStreamErrors() {
	return 0;
}

std::size_t UartSource::loadFromQueue(sdm_sample_t *data,std::size_t n) {
	if(n==0) return 0;

	std::size_t current=0;

	while(current<n&&!_q.empty()) {
		if((_q.front()&0x80)==0) { // MSB not set, skip this byte
			_q.pop_front();
			continue;
		}
		if(_q.size()<2) break; // packet size is 2 bytes
		int sample=((_q[0]&0x1F)<<5)|(_q[1]&0x1F);
		data[current]=static_cast<sdm_sample_t>(sample);
		current++;
		_q.erase(_q.begin(),_q.begin()+1);
	}
	
	return current; // number of samples loaded from the queue
}
