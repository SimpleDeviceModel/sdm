/*
 * Copyright (c) 2022 by Alex I. Kuznetsov
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
 * This module implements the Dcl plugin classes.
 */

#include "dcl.h"

#define UART_TIMEOUT_MSEC 5000

#include <vector>
#include <stdexcept>
#include <algorithm>

/*
 * DclPlugin instance
 * 
 * This static member function returns a pointer to the DclPlugin instance.
 * The object itself is defined as a static local variable to ensure
 * that it is initialized before use, thus avoiding the so-called "static
 * initialization order fiasco".
 */

SDMAbstractPlugin *SDMAbstractPlugin::instance() {
	static DclPlugin plugin;
	return &plugin;
}

/*
 * DclPlugin members
 */

DclPlugin::DclPlugin() {
	addConstProperty("Name","Debug Communication Link");
	addConstProperty("Vendor","Alex I. Kuznetsov");
	addListItem("Devices","AHB UART");
}

SDMAbstractDevice *DclPlugin::openDevice(int id) {
	if(id!=0) throw std::runtime_error("No device with such ID");
	return new DclDevice();
}

/*
 * DclDevice members
 */

DclDevice::DclDevice() {
	addConstProperty("Name","AHB UART");
// Make sdmconsole open channels and sources automatically
	addConstProperty("AutoOpenChannels","open");
	addConstProperty("AutoOpenSources","open");
	
	addListItem("ConnectionParameters","SerialPort");
	addListItem("ConnectionParameters","BaudRate");
	addListItem("Channels","Bus");
	
// Try to auto detect serial port
#ifdef _WIN32
	std::string defaultPort("COM1");
#else
	std::string defaultPort("/dev/ttyUSB0");
#endif
	
	auto portlist=Uart::listSerialPorts();
	if(!portlist.empty()) defaultPort=portlist[0];
	
	addProperty("SerialPort",defaultPort);
	addProperty("BaudRate","115200");
}

int DclDevice::close() {
	delete this;
	return 0;
}

SDMAbstractChannel *DclDevice::openChannel(int id) {
	if(id!=0) throw std::runtime_error("No channel with such ID");
	return new DclChannel(_port);
}

int DclDevice::connect() {
	_port.open(getProperty("SerialPort"));
	_port.setBaudRate(std::stoi(getProperty("BaudRate")));
	_port.setDataBits(8);
	_port.setStopBits(Uart::OneStop);
	_port.setParity(Uart::NoParity);
	_port.setFlowControl(Uart::NoFlowControl);
	return 0;
}

int DclDevice::disconnect() {
	_port.close();
	return 0;
}

int DclDevice::getConnectionStatus() {
	if(_port) return 1;
	return 0;
}

/*
 * DclChannel members
 */

DclChannel::DclChannel(Uart &port): _port(port) {
	addConstProperty("Name","Bus");
}

int DclChannel::close() {
	delete this;
	return 0;
}

int DclChannel::writeReg(sdm_addr_t addr,sdm_reg_t data) {
	writeMem(addr,&data,1);
	return 0;
}

sdm_reg_t DclChannel::readReg(sdm_addr_t addr,int *status) {
	sdm_reg_t data;
	readMem(addr,&data,1);
	return data;
}

int DclChannel::writeMem(sdm_addr_t addr,const sdm_reg_t *data,std::size_t n) {
	while(n>0) {
		Byte towrite=static_cast<Byte>(std::min<std::size_t>(n,64));
		std::vector<Byte> packet(5+towrite*4);
		packet[0]=0xC0|(towrite-1);
		packet[1]=(addr>>24)&0xFF;
		packet[2]=(addr>>16)&0xFF;
		packet[3]=(addr>>8)&0xFF;
		packet[4]=(addr)&0xFF;
		for(std::size_t i=0;i<towrite;i++) {
			auto word=*data;
			packet[i*4+5]=(word>>24)&0xFF;
			packet[i*4+6]=(word>>16)&0xFF;
			packet[i*4+7]=(word>>8)&0xFF;
			packet[i*4+8]=(word)&0xFF;
			data++;
		}
		sendBytes(packet);
		addr+=(4*towrite);
		n-=towrite;
	}
	return 0;
}

int DclChannel::readMem(sdm_addr_t addr,sdm_reg_t *data,std::size_t n) {
	while(n>0) {
		Byte toread=static_cast<Byte>(std::min<std::size_t>(n,64));
		std::vector<Byte> packet(5);
		packet[0]=0x80|(toread-1);
		packet[1]=(addr>>24)&0xFF;
		packet[2]=(addr>>16)&0xFF;
		packet[3]=(addr>>8)&0xFF;
		packet[4]=(addr)&0xFF;
		sendBytes(packet);
		auto const &r=recvBytes(toread*4);
		for(std::size_t i=0;i<toread;i++) {
			*data=0;
			*data|=(r[i*4]<<24);
			*data|=(r[i*4+1]<<16);
			*data|=(r[i*4+2]<<8);
			*data|=(r[i*4+3]);
			data++;
		}
		addr+=(4*toread);
		n-=toread;
	}
	return 0;
}

void DclChannel::sendBytes(const std::vector<Byte> &s) {
	auto p=reinterpret_cast<const char*>(s.data());
	std::size_t towrite=s.size();
	while(towrite>0) {
		std::size_t r=_port.write(p,towrite);
		p+=r;
		towrite-=r;
	}
}

std::vector<Byte> DclChannel::recvBytes(std::size_t n) {
	std::vector<Byte> data(n);
	auto p=reinterpret_cast<char *>(data.data());
	while(n>0) {
		std::size_t r=_port.read(p,n,UART_TIMEOUT_MSEC);
		if(r==0) throw std::runtime_error("Device is not responding");
		p+=r;
		n-=r;
	}
	return data;
}
