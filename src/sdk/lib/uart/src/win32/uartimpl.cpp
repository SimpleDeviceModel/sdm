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
 * on Win32 API.
 */

#include "uartimpl.h"

#include <stdexcept>
#include <algorithm>

UartImpl::UartImpl(const std::string &portName) {
	ZeroMemory(&_commTimeouts,sizeof(COMMTIMEOUTS));
// Prepend the port name with the "\\.\" prefix, if it doesn't already start with a backslash
	auto name=portName;
	if(!name.empty()&&name[0]!='\\') name="\\\\.\\"+name;
	
// Open the port
	_hPort=CreateFile(name.c_str(),GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	if(_hPort==INVALID_HANDLE_VALUE)
		throw std::runtime_error("Cannot open serial port \""+name+
			"\": Windows error code "+std::to_string(GetLastError()));
	
// Terminate ongoing activity (if any) and clear buffers
	PurgeComm(_hPort,PURGE_RXABORT|PURGE_RXCLEAR|PURGE_TXABORT|PURGE_TXCLEAR);

// Get system serial port configuration
	BOOL b=GetCommState(_hPort,&_currentDCB);
	if(!b) throw std::runtime_error("Cannot get serial port state");
	
// Try to detect serial port settings (we don't support all modes)
	_baudRate=static_cast<int>(_currentDCB.BaudRate);
	_dataBits=static_cast<int>(_currentDCB.ByteSize);
	
	switch(_currentDCB.StopBits) {
	case ONESTOPBIT:
		_stopBits=Uart::OneStop;
		break;
	case TWOSTOPBITS:
		_stopBits=Uart::TwoStops;
		break;
	default: // we don't support 1.5 stop bits
		_stopBits=Uart::OneStop;
		break;
	}
	
	if(!_currentDCB.fParity) _parity=Uart::NoParity;
	else if(_currentDCB.Parity==EVENPARITY) _parity=Uart::EvenParity;
	else if(_currentDCB.Parity==ODDPARITY) _parity=Uart::OddParity;
	else _parity=Uart::NoParity; // other parity modes are not supported
	
	if(_currentDCB.fInX||_currentDCB.fOutX)
		_flowControl=Uart::SoftwareFlowControl;
	else if(_currentDCB.fRtsControl==RTS_CONTROL_HANDSHAKE||_currentDCB.fOutxCtsFlow)
		_flowControl=Uart::HardwareFlowControl;
	else _flowControl=Uart::NoFlowControl;
	
	commitSettings();
}

UartImpl::~UartImpl() {
	CloseHandle(_hPort);
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
// Set timeouts
	if(timeout>0) {
		_commTimeouts.WriteTotalTimeoutMultiplier=0;
		_commTimeouts.WriteTotalTimeoutConstant=static_cast<DWORD>(timeout);
	}
	else if(timeout==0) { // emulate non-blocking mode, set some sensible values
		_commTimeouts.WriteTotalTimeoutMultiplier=1;
		_commTimeouts.WriteTotalTimeoutConstant=10;
	}
	else if(timeout==-1) { // block indefinitely
		_commTimeouts.WriteTotalTimeoutMultiplier=0;
		_commTimeouts.WriteTotalTimeoutConstant=0;
	}
	SetCommTimeouts(_hPort,&_commTimeouts);
	
// Perform write
	DWORD dwBytesWritten;
	BOOL b=WriteFile(_hPort,buf,static_cast<DWORD>(n),&dwBytesWritten,NULL);
	if(!b) throw std::runtime_error("Serial port write failed");
	
	return static_cast<std::size_t>(dwBytesWritten);
}

std::size_t UartImpl::read(char *buf,std::size_t n,int timeout) {
// Set timeouts
	if(timeout>0) {
		_commTimeouts.ReadIntervalTimeout=0;
		_commTimeouts.ReadTotalTimeoutMultiplier=0;
		_commTimeouts.ReadTotalTimeoutConstant=static_cast<DWORD>(timeout);
	}
	else if(timeout==0) { // non-blocking mode
		_commTimeouts.ReadIntervalTimeout=MAXDWORD;
		_commTimeouts.ReadTotalTimeoutMultiplier=0;
		_commTimeouts.ReadTotalTimeoutConstant=0;
	}
	else if(timeout==-1) { // block indefinitely
		_commTimeouts.ReadIntervalTimeout=MAXDWORD;
		_commTimeouts.ReadTotalTimeoutMultiplier=MAXDWORD;
		_commTimeouts.ReadTotalTimeoutConstant=MAXDWORD-1;
	}
	SetCommTimeouts(_hPort,&_commTimeouts);
	
// Perform read
	DWORD dwBytesRead;
	do {
		BOOL b=ReadFile(_hPort,buf,static_cast<DWORD>(n),&dwBytesRead,NULL);
		if(!b) throw std::runtime_error("Serial port read failed");
	} while(timeout==-1&&!dwBytesRead);
	
	return static_cast<std::size_t>(dwBytesRead);
}

void UartImpl::setDTR(bool b) {
	BOOL r=EscapeCommFunction(_hPort,b?SETDTR:CLRDTR);
	if(!r) throw std::runtime_error("Cannot set DTR line status");
}

bool UartImpl::getDSR() const {
	DWORD st;
	BOOL r=GetCommModemStatus(_hPort,&st);
	if(!r) throw std::runtime_error("Cannot get DSR line status");
	return ((st&MS_DSR_ON)!=0);
}

void UartImpl::setRTS(bool b) {
	if(_flowControl==Uart::HardwareFlowControl)
		throw std::runtime_error("Cannot set RTS line status manually when hardware flow control is active");
	BOOL r=EscapeCommFunction(_hPort,b?SETRTS:CLRRTS);
	if(!r) throw std::runtime_error("Cannot set RTS line status");
}

bool UartImpl::getCTS() const {
	DWORD st;
	BOOL r=GetCommModemStatus(_hPort,&st);
	if(!r) throw std::runtime_error("Cannot get CTS line status");
	return ((st&MS_CTS_ON)!=0);
}

std::vector<std::string> UartImpl::listSerialPorts() {
	std::vector<std::string> res;
	
	HKEY key;
	LONG r=RegOpenKeyEx(HKEY_LOCAL_MACHINE,"HARDWARE\\DEVICEMAP\\SERIALCOMM",0,KEY_READ,&key);
	if(r!=ERROR_SUCCESS) return res;
	
	DWORD values,maxNameLen,maxDataLen;
	r=RegQueryInfoKey(key,NULL,NULL,NULL,NULL,NULL,NULL,&values,&maxNameLen,&maxDataLen,NULL,NULL);
	if(r!=ERROR_SUCCESS) {
		RegCloseKey(key);
		return res;
	}
	
	std::vector<char> name(maxNameLen+1);
	std::vector<char> data(maxDataLen+1);
	
	for(DWORD d=0;d<values;d++) {
		DWORD nameBufSize=maxNameLen+1;
		DWORD dataBufSize=maxDataLen+1;
		DWORD type;
		r=RegEnumValue(key,d,name.data(),&nameBufSize,NULL,&type,reinterpret_cast<BYTE*>(data.data()),&dataBufSize);
		if(r!=ERROR_SUCCESS) break;
		if(type!=REG_SZ) continue;
		while(!data.empty()&&data[dataBufSize-1]=='\0') dataBufSize--;
		res.emplace_back(data.data(),dataBufSize);
	}
	
	RegCloseKey(key);
	
// Sort the table: COM ports by number, everything else - alphabetically
	auto compFunctor=[](const std::string &left,const std::string &right){
		if((left.substr(0,3)!="COM")||(right.substr(0,3)!="COM")) return (left<right);
		try {
			auto leftNum=std::stoi(left.substr(3));
			auto rightNum=std::stoi(right.substr(3));
			return (leftNum<rightNum);
		}
		catch(std::exception &) {
			return (left<right);
		}
	};
	std::sort(res.begin(),res.end(),compFunctor);
	
	return res;
}

/*
 * Private members
 */

void UartImpl::commitSettings() {
	auto dcb=_currentDCB;
	
// Permanent settings
	dcb.fBinary=TRUE;
	dcb.fDtrControl=DTR_CONTROL_ENABLE; // assert DTR, just in case
	dcb.fDsrSensitivity=FALSE;
	dcb.fOutxDsrFlow=FALSE;
	dcb.fErrorChar=FALSE;
	dcb.fNull=FALSE;
	dcb.fAbortOnError=FALSE;
	dcb.fTXContinueOnXoff=FALSE;
	
// Mutable settings
	dcb.BaudRate=static_cast<DWORD>(_baudRate);
	dcb.ByteSize=static_cast<BYTE>(_dataBits);
	
	if(_stopBits==Uart::OneStop) dcb.StopBits=ONESTOPBIT;
	else dcb.StopBits=TWOSTOPBITS;
	
	if(_parity==Uart::NoParity) {
		dcb.fParity=FALSE;
		dcb.Parity=NOPARITY;
	}
	else if(_parity==Uart::EvenParity) {
		dcb.fParity=TRUE;
		dcb.Parity=EVENPARITY;
	}
	else {
		dcb.fParity=TRUE;
		dcb.Parity=ODDPARITY;
	}
	
	if(_flowControl==Uart::NoFlowControl) {
		dcb.fOutX=FALSE;
		dcb.fInX=FALSE;
		dcb.fOutxCtsFlow=FALSE;
		dcb.fRtsControl=RTS_CONTROL_ENABLE; // assert RTS, just in case
	}
	else if(_flowControl==Uart::HardwareFlowControl) {
		dcb.fOutX=FALSE;
		dcb.fInX=FALSE;
		dcb.fOutxCtsFlow=TRUE;
		dcb.fRtsControl=RTS_CONTROL_HANDSHAKE;
	}
	else { // software flow control
		dcb.fOutX=TRUE;
		dcb.fInX=TRUE;
		dcb.fOutxCtsFlow=FALSE;
		dcb.fRtsControl=RTS_CONTROL_ENABLE; // assert RTS, just in case
	}
	
	BOOL b=SetCommState(_hPort,&dcb);
	if(!b) throw std::runtime_error("Cannot set serial port state");
	
	_currentDCB=dcb;
}
