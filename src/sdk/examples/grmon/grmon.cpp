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
 * This module implements the Grmon plugin classes.
 */

#include "grmon.h"

#include <vector>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <algorithm>

/*
 * GrmonPlugin instance
 * 
 * This static member function returns a pointer to the GrmonPlugin instance.
 * The object itself is defined as a static local variable to ensure
 * that it is initialized before use, thus avoiding the so-called "static
 * initialization order fiasco".
 */

SDMAbstractPlugin *SDMAbstractPlugin::instance() {
	static GrmonPlugin plugin;
	return &plugin;
}

/*
 * GrmonPlugin members
 */

GrmonPlugin::GrmonPlugin() {
	addConstProperty("Name","GRMON protocol plugin");
	addConstProperty("Vendor","Alex I. Kuznetsov");
	addListItem("Devices","GRMON AHBUART");
}

SDMAbstractDevice *GrmonPlugin::openDevice(int id) {
	if(id!=0) throw std::runtime_error("No device with such ID");
	return new GrmonDevice();
}

/*
 * GrmonDevice members
 */

GrmonDevice::GrmonDevice() {
	addConstProperty("Name","GRMON AHBUART");
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

int GrmonDevice::close() {
	delete this;
	return 0;
}

SDMAbstractChannel *GrmonDevice::openChannel(int id) {
	if(id!=0) throw std::runtime_error("No channel with such ID");
	return new GrmonChannel(_port);
}

int GrmonDevice::connect() {
	_port.open(getProperty("SerialPort"));
	_port.setBaudRate(std::stoi(getProperty("BaudRate")));
	_port.setDataBits(8);
	_port.setStopBits(Uart::OneStop);
	_port.setParity(Uart::NoParity);
	_port.setFlowControl(Uart::NoFlowControl);
	return 0;
}

int GrmonDevice::disconnect() {
	_port.close();
	return 0;
}

int GrmonDevice::getConnectionStatus() {
	if(_port) return 1;
	return 0;
}

/*
 * GrmonChannel members
 */

GrmonChannel::GrmonChannel(Uart &port): _port(port) {
	addConstProperty("Name","Bus");
}

int GrmonChannel::close() {
	delete this;
	return 0;
}

int GrmonChannel::writeReg(sdm_addr_t addr,sdm_reg_t data) {
// Send "Write register" command packet: 11 (LENGTH-1)[5:0] ADDR[31:0] DATA[31:0]
	std::vector<Byte> packet(9);
	packet[0]=0xC0;
	packet[1]=(addr>>24)&0xFF;
	packet[2]=(addr>>16)&0xFF;
	packet[3]=(addr>>8)&0xFF;
	packet[4]=(addr)&0xFF;
	packet[5]=(data>>24)&0xFF;
	packet[6]=(data>>16)&0xFF;
	packet[7]=(data>>8)&0xFF;
	packet[8]=(data)&0xFF;
	sendBytes(packet);
	return 0;
}

sdm_reg_t GrmonChannel::readReg(sdm_addr_t addr,int *status) {
// Send "Read register" command packet: 10 (LENGTH-1)[5:0] ADDR[31:0]
	std::vector<Byte> packet(5);
	packet[0]=0x80;
	packet[1]=(addr>>24)&0xFF;
	packet[2]=(addr>>16)&0xFF;
	packet[3]=(addr>>8)&0xFF;
	packet[4]=(addr)&0xFF;
	sendBytes(packet);
// Receive data
	auto const &r=recvBytes(4);
	sdm_reg_t data=0;
	for(int i=0;i<4;i++) {
		data=data<<8;
		data|=r[i];
	}
	return data;
}

void GrmonChannel::sendBytes(const std::vector<Byte> &s) {
	auto p=reinterpret_cast<const char*>(s.data());
	std::size_t towrite=s.size();
	while(towrite>0) {
		std::size_t r=_port.write(p,towrite);
		p+=r;
		towrite-=r;
	}
}

std::vector<GrmonChannel::Byte> GrmonChannel::recvBytes(std::size_t n) {
	std::vector<Byte> data(n);
	auto p=reinterpret_cast<char *>(data.data());
	while(n>0) {
		std::size_t r=_port.read(p,n);
		p+=r;
		n-=r;
	}
	return data;
}
