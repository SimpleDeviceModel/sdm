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
 * This module provides an implementation of the UartImpl class based
 * on POSIX termios API.
 */

#include "uartimpl.h"

#include <stdexcept>
#include <cstring>
#include <algorithm>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

UartImpl::UartImpl(const std::string &portName) {
	_port=::open(portName.c_str(),O_RDWR|O_NOCTTY|O_NONBLOCK|O_NDELAY);
	if(_port==-1) throw std::runtime_error("Cannot open serial port \""+portName+"\": "+strerror(errno));
	
// Get system serial port configuration
	int r=tcgetattr(_port,&_currentTermios);
	if(r) throw std::runtime_error("Cannot get serial port state");
	
// Try to detect serial port settings (we don't support all modes)
	auto speed=cfgetospeed(&_currentTermios);
	switch(speed) {
	case B50:
		_baudRate=50;
		break;
	case B75:
		_baudRate=75;
		break;
	case B110:
		_baudRate=110;
		break;
	case B134:
		_baudRate=134;
		break;
	case B150:
		_baudRate=150;
		break;
	case B200:
		_baudRate=200;
		break;
	case B300:
		_baudRate=300;
		break;
	case B600:
		_baudRate=600;
		break;
	case B1200:
		_baudRate=1200;
		break;
	case B1800:
		_baudRate=1800;
		break;
	case B2400:
		_baudRate=2400;
		break;
	case B4800:
		_baudRate=4800;
		break;
	case B9600:
		_baudRate=9600;
		break;
	case B19200:
		_baudRate=19200;
		break;
	case B38400:
		_baudRate=38400;
		break;
// Non-POSIX baud rates: need to check for macros
#ifdef B7200
	case B7200:
		_baudRate=7200;
		break;
#endif
#ifdef B14400
	case B14400:
		_baudRate=14400;
		break;
#endif
#ifdef B28800
	case B28800:
		_baudRate=28800;
		break;
#endif
#ifdef B57600
	case B57600:
		_baudRate=57600;
		break;
#endif
#ifdef B76800
	case B76800:
		_baudRate=76800;
		break;
#endif
#ifdef B115200
	case B115200:
		_baudRate=115200;
		break;
#endif
#ifdef B230400
	case B230400:
		_baudRate=230400;
		break;
#endif
#ifdef B460800
	case B460800:
		_baudRate=460800;
		break;
#endif
#ifdef B500000
	case B500000:
		_baudRate=500000;
		break;
#endif
#ifdef B576000
	case B576000:
		_baudRate=576000;
		break;
#endif
#ifdef B921600
	case B921600:
		_baudRate=921600;
		break;
#endif
#ifdef B1000000
	case B1000000:
		_baudRate=1000000;
		break;
#endif
#ifdef B1152000
	case B1152000:
		_baudRate=1152000;
		break;
#endif
#ifdef B1500000
	case B1500000:
		_baudRate=1500000;
		break;
#endif
#ifdef B2000000
	case B2000000:
		_baudRate=2000000;
		break;
#endif
#ifdef B2500000
	case B2500000:
		_baudRate=2500000;
		break;
#endif
#ifdef B3000000
	case B3000000:
		_baudRate=3000000;
		break;
#endif
#ifdef B3500000
	case B3500000:
		_baudRate=3500000;
		break;
#endif
#ifdef B4000000
	case B4000000:
		_baudRate=4000000;
		break;
#endif
	default:
		_baudRate=9600;
		break;
	}
	
	switch(_currentTermios.c_cflag&CSIZE) {
	case CS5:
		_dataBits=5;
		break;
	case CS6:
		_dataBits=6;
		break;
	case CS7:
		_dataBits=7;
		break;
	case CS8:
		_dataBits=8;
		break;
	default:
		_dataBits=8;
		break;
	}
	
	if(_currentTermios.c_cflag&CSTOPB) _stopBits=Uart::TwoStops;
	else _stopBits=Uart::OneStop;
	
	if(!(_currentTermios.c_cflag&PARENB)) _parity=Uart::NoParity;
	else if(!(_currentTermios.c_cflag&PARODD)) _parity=Uart::EvenParity;
	else _parity=Uart::OddParity;
	
	if((_currentTermios.c_iflag&IXON)||(_currentTermios.c_iflag&IXOFF))
		_flowControl=Uart::SoftwareFlowControl;
	else if(_currentTermios.c_cflag&CRTSCTS)
		_flowControl=Uart::HardwareFlowControl;
	else _flowControl=Uart::NoFlowControl;
	
	commitSettings();
}

UartImpl::~UartImpl() {
	::close(_port);
}

int UartImpl::baudRate() const {
	return _baudRate;
}

void UartImpl::setBaudRate(int i) {
	auto old=_baudRate;
	_baudRate=i;
	try {
		commitSettings();
	}
	catch(std::exception &) {
		_baudRate=old;
		throw;
	}
}

int UartImpl::dataBits() const {
	return _dataBits;
}

void UartImpl::setDataBits(int i) {
	auto old=_dataBits;
	_dataBits=i;
	try {
		commitSettings();
	}
	catch(std::exception &) {
		_dataBits=old;
		throw;
	}
}

Uart::StopBits UartImpl::stopBits() const {
	return _stopBits;
}

void UartImpl::setStopBits(Uart::StopBits i) {
	auto old=_stopBits;
	_stopBits=i;
	try {
		commitSettings();
	}
	catch(std::exception &) {
		_stopBits=old;
		throw;
	}
}

Uart::Parity UartImpl::parity() const {
	return _parity;
}

void UartImpl::setParity(Uart::Parity p) {
	auto old=_parity;
	_parity=p;
	try {
		commitSettings();
	}
	catch(std::exception &) {
		_parity=old;
		throw;
	}
}

Uart::FlowControl UartImpl::flowControl() const {
	return _flowControl;
}

void UartImpl::setFlowControl(Uart::FlowControl f) {
	auto old=_flowControl;
	_flowControl=f;
	try {
		commitSettings();
	}
	catch(std::exception &) {
		_flowControl=old;
		throw;
	}
}

std::size_t UartImpl::write(const char *buf,std::size_t n,int timeout) {
// Wait if blocking operation is requested
	if(timeout!=0) {
		fd_set set;
		FD_ZERO(&set);
		FD_SET(_port,&set);
		
		struct timeval tv;
		struct timeval *ptv=nullptr;
		if(timeout>=0) {
			tv.tv_sec=timeout/1000;
			tv.tv_usec=1000*(timeout%1000);
			ptv=&tv;
		}
		
		int r;
		do {
			r=select(_port+1,NULL,&set,NULL,ptv);
		} while(r==-1&&errno==EINTR); // select() can be spuriously interrupted by signals
	}
	
// Perform write
	int r=::write(_port,buf,n);
	if(r==-1) throw std::runtime_error("Serial port write failed");
	
	return static_cast<std::size_t>(r);
}

std::size_t UartImpl::read(char *buf,std::size_t n,int timeout) {
// Wait if blocking operation is requested
	if(timeout!=0) {
		fd_set set;
		FD_ZERO(&set);
		FD_SET(_port,&set);
		
		struct timeval tv;
		struct timeval *ptv=nullptr;
		if(timeout>=0) {
			tv.tv_sec=timeout/1000;
			tv.tv_usec=1000*(timeout%1000);
			ptv=&tv;
		}
		
		int r;
		do {
			r=select(_port+1,&set,NULL,NULL,ptv);
		} while(r==-1&&errno==EINTR); // select() can be spuriously interrupted by signals
	}
	
// Perform write
	int r=::read(_port,buf,n);
	if(r==-1) throw std::runtime_error("Serial port read failed");
	
	return static_cast<std::size_t>(r);
}

void UartImpl::setDTR(bool b) {
	int status;
	int r=ioctl(_port,TIOCMGET,&status);
	if(r==-1) throw std::runtime_error("Cannot set DTR line status");
	status&=~TIOCM_DTR;
	if(b) status|=TIOCM_DTR;
	r=ioctl(_port,TIOCMSET,status);
	if(r==-1) throw std::runtime_error("Cannot set DTR line status");
}

bool UartImpl::getDSR() const {
	int status;
	int r=ioctl(_port,TIOCMGET,&status);
	if(r==-1) throw std::runtime_error("Cannot get DSR line status");
	return ((status&TIOCM_DSR)!=0);
}

void UartImpl::setRTS(bool b) {
	if(_flowControl==Uart::HardwareFlowControl)
		throw std::runtime_error("Cannot set RTS line status manually when hardware flow control is active");
	
	int status;
	int r=ioctl(_port,TIOCMGET,&status);
	if(r==-1) throw std::runtime_error("Cannot set RTS line status");
	status&=~TIOCM_RTS;
	if(b) status|=TIOCM_RTS;
	r=ioctl(_port,TIOCMSET,status);
	if(r==-1) throw std::runtime_error("Cannot set RTS line status");
}

bool UartImpl::getCTS() const {
	int status;
	int r=ioctl(_port,TIOCMGET,&status);
	if(r==-1) throw std::runtime_error("Cannot get CTS line status");
	return ((status&TIOCM_CTS)!=0);
}

std::vector<std::string> UartImpl::listSerialPorts() {
	std::vector<std::string> res;
	
	DIR *dir=opendir("/dev");
	if(!dir) return res;
	
	for(;;) {
		auto entry=readdir(dir);
		if(!entry) break;
		auto const prefix=std::string(entry->d_name).substr(0,3);
		if(prefix=="tty"||prefix=="cua") { // "cua" is used on BSD systems
			std::string path("/dev/");
			path+=entry->d_name;
// Try to open it
			int fd=::open(path.c_str(),O_RDWR|O_NOCTTY|O_NONBLOCK|O_NDELAY);
			if(fd==-1) continue;
// Try to obtain termios structure
			struct termios tmp;
			int r=tcgetattr(fd,&tmp);
			if(r==-1) {
				::close(fd);
				continue;
			}
// Try to obtain modem status bits
			int status;
			r=ioctl(fd,TIOCMGET,&status);
			if(r==-1) {
				::close(fd);
				continue;
			}

// OK, this seems to be a serial port
			::close(fd);
			
			res.emplace_back(path);
		}
	}
	
	closedir(dir);
	
	std::sort(res.begin(),res.end());
	
	return res;
}

/*
 * Private members
 */

void UartImpl::commitSettings() {
	auto tos=_currentTermios;
	
// Permanent settings
	tos.c_iflag&=~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
	tos.c_oflag&=~OPOST;
	tos.c_lflag&=~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
	tos.c_cc[VMIN]=0;
	tos.c_cc[VTIME]=0;
	
// Populate baud rate
	switch(_baudRate) {
	case 50:
		cfsetospeed(&tos,B50);
		break;
	case 75:
		cfsetospeed(&tos,B75);
		break;
	case 110:
		cfsetospeed(&tos,B110);
		break;
	case 134:
		cfsetospeed(&tos,B134);
		break;
	case 150:
		cfsetospeed(&tos,B150);
		break;
	case 200:
		cfsetospeed(&tos,B200);
		break;
	case 300:
		cfsetospeed(&tos,B300);
		break;
	case 600:
		cfsetospeed(&tos,B600);
		break;
	case 1200:
		cfsetospeed(&tos,B1200);
		break;
	case 1800:
		cfsetospeed(&tos,B1800);
		break;
	case 2400:
		cfsetospeed(&tos,B2400);
		break;
	case 4800:
		cfsetospeed(&tos,B4800);
		break;
	case 9600:
		cfsetospeed(&tos,B9600);
		break;
	case 19200:
		cfsetospeed(&tos,B19200);
		break;
	case 38400:
		cfsetospeed(&tos,B38400);
		break;
// Non-POSIX baud rates - need to check macros
#ifdef B7200
	case 7200:
		cfsetospeed(&tos,B7200);
		break;
#endif
#ifdef B14400
	case 14400:
		cfsetospeed(&tos,B14400);
		break;
#endif
#ifdef B28800
	case 28800:
		cfsetospeed(&tos,B28800);
		break;
#endif
#ifdef B57600
	case 57600:
		cfsetospeed(&tos,B57600);
		break;
#endif
#ifdef B76800
	case 76800:
		cfsetospeed(&tos,B76800);
		break;
#endif
#ifdef B115200
	case 115200:
		cfsetospeed(&tos,B115200);
		break;
#endif
#ifdef B230400
	case 230400:
		cfsetospeed(&tos,B230400);
		break;
#endif
#ifdef B460800
	case 460800:
		cfsetospeed(&tos,B460800);
		break;
#endif
#ifdef B500000
	case 500000:
		cfsetospeed(&tos,B500000);
		break;
#endif
#ifdef B576000
	case 576000:
		cfsetospeed(&tos,B576000);
		break;
#endif
#ifdef B921600
	case 921600:
		cfsetospeed(&tos,B921600);
		break;
#endif
#ifdef B1000000
	case 1000000:
		cfsetospeed(&tos,B1000000);
		break;
#endif
#ifdef B1152000
	case 1152000:
		cfsetospeed(&tos,B1152000);
		break;
#endif
#ifdef B1500000
	case 1500000:
		cfsetospeed(&tos,B1500000);
		break;
#endif
#ifdef B2000000
	case 2000000:
		cfsetospeed(&tos,B2000000);
		break;
#endif
#ifdef B2500000
	case 2500000:
		cfsetospeed(&tos,B2500000);
		break;
#endif
#ifdef B3000000
	case 3000000:
		cfsetospeed(&tos,B3000000);
		break;
#endif
#ifdef B3500000
	case 3500000:
		cfsetospeed(&tos,B3500000);
		break;
#endif
#ifdef B4000000
	case 4000000:
		cfsetospeed(&tos,B4000000);
		break;
#endif
	default:
		throw std::runtime_error("Bad baud rate value");
	}
	
	cfsetispeed(&tos,0); // input baud rate is the same as output
	
// Populate character size
	tos.c_cflag&=(~CSIZE);
	switch(_dataBits) {
	case 5:
		tos.c_cflag|=CS5;
		break;
	case 6:
		tos.c_cflag|=CS6;
		break;
	case 7:
		tos.c_cflag|=CS7;
		break;
	case 8:
		tos.c_cflag|=CS8;
		break;
	default:
		throw std::runtime_error("Bad character size");
	}

// Populate stop bits
	tos.c_cflag&=~CSTOPB;
	if(_stopBits==Uart::TwoStops) tos.c_cflag|=CSTOPB;

// Populate parity
	if(_parity==Uart::NoParity) {
		tos.c_cflag&=~PARENB;
		tos.c_iflag&=~INPCK;
	}
	else if(_parity==Uart::EvenParity) {
		tos.c_cflag|=PARENB;
		tos.c_cflag&=~PARODD;
		tos.c_iflag|=INPCK;
	}
	else {
		tos.c_cflag|=PARENB;
		tos.c_cflag|=PARODD;
		tos.c_iflag|=INPCK;
	}

// Populate flow control
	if(_flowControl==Uart::NoFlowControl) {
		tos.c_iflag&=~(IXON|IXOFF);
		tos.c_cflag&=~CRTSCTS;
	}
	else if(_flowControl==Uart::HardwareFlowControl) {
		tos.c_iflag&=~(IXON|IXOFF);
		tos.c_cflag|=CRTSCTS;
	}
	else { // software flow control
		tos.c_iflag|=(IXON|IXOFF);
		tos.c_cflag&=~CRTSCTS;
	}
	
// Apply settings
	int r=tcsetattr(_port,TCSANOW,&tos);
	if(r) throw std::runtime_error("Cannot set serial port state");
	
	_currentTermios=tos;
}
