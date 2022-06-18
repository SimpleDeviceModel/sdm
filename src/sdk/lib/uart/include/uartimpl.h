/*
 * Copyright (c) 2015-2022 Simple Device Model contributors
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
 * This header file defines a class that implements serial port access
 * using Win32 API.
 */

#ifndef UARTIMPL_H_INCLUDED
#define UARTIMPL_H_INCLUDED

#include "uart.h"

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
	#include <termios.h>
#endif

class UartImpl {
	int _baudRate=9600;
	int _dataBits=8;
	Uart::StopBits _stopBits=Uart::OneStop;
	Uart::Parity _parity=Uart::NoParity;
	Uart::FlowControl _flowControl=Uart::NoFlowControl;

#ifdef _WIN32
	DCB _currentDCB;
	COMMTIMEOUTS _commTimeouts;
	HANDLE _hPort;
#else
	int _port;
	struct termios _currentTermios;
#endif

public:
	UartImpl(const std::string &portName);
	UartImpl(const UartImpl &)=delete;
	UartImpl(UartImpl &&orig)=delete;
	~UartImpl();
	
	UartImpl &operator=(const UartImpl &)=delete;
	UartImpl &operator=(UartImpl &&orig)=delete;
	
	int baudRate() const;
	void setBaudRate(int i);
	
	int dataBits() const;
	void setDataBits(int i);
	
	Uart::StopBits stopBits() const;
	void setStopBits(Uart::StopBits s);
	
	Uart::Parity parity() const;
	void setParity(Uart::Parity p);
	
	Uart::FlowControl flowControl() const;
	void setFlowControl(Uart::FlowControl f);
	
	std::size_t write(const char *buf,std::size_t n,int timeout);
	std::size_t read(char *buf,std::size_t n,int timeout);
	
	void setDTR(bool b);
	bool getDSR() const;
	
	void setRTS(bool b);
	bool getCTS() const;
	
	static std::vector<std::string> listSerialPorts();

private:
	void commitSettings();
};

#endif
