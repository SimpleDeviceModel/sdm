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
 * This module provides an implementation of the Uart class members.
 */

#include "uart.h"

#include "uartimpl.h"

#include <vector>
#include <utility>
#include <stdexcept>

Uart::Uart(): _impl(nullptr),_eol(0) {}

Uart::Uart(Uart &&orig): _impl(orig._impl),_eol(orig._eol) {
	orig._impl=nullptr;
	orig._eol=0;
}

Uart::~Uart() {
	if(_impl) delete _impl;
}

Uart &Uart::operator=(Uart &&orig) {
	swap(orig);
	return *this;
}

void Uart::swap(Uart &other) {
	std::swap(_impl,other._impl);
	std::swap(_eol,other._eol);
}

Uart::operator bool() const {
	return (_impl!=nullptr);
}

void Uart::open(const std::string &portName) {
	if(_impl) throw std::runtime_error("Port is already opened");
	_impl=new UartImpl(portName);
}

void Uart::close() {
	(void)impl();
	operator=(Uart());
}

int Uart::baudRate() const {
	return impl()->baudRate();
}

void Uart::setBaudRate(int i) {
	impl()->setBaudRate(i);
}

int Uart::dataBits() const {
	return impl()->dataBits();
}

void Uart::setDataBits(int i) {
	impl()->setDataBits(i);
}

Uart::StopBits Uart::stopBits() const {
	return impl()->stopBits();
}

void Uart::setStopBits(StopBits s) {
	impl()->setStopBits(s);
}

Uart::Parity Uart::parity() const {
	return impl()->parity();
}

void Uart::setParity(Parity p) {
	impl()->setParity(p);
}

Uart::FlowControl Uart::flowControl() const {
	return impl()->flowControl();
}

void Uart::setFlowControl(FlowControl f) {
	impl()->setFlowControl(f);
}

std::size_t Uart::write(const char *buf,std::size_t n,int timeout) {
	return impl()->write(buf,n,timeout);
}

std::size_t Uart::read(char *buf,std::size_t n,int timeout) {
	return impl()->read(buf,n,timeout);
}

std::string Uart::readLine() {
	std::string str;
	char ch;
	for(;;) {
		impl()->read(&ch,1,-1);
		if((ch=='\x0D')||(ch=='\x0A')) {
			if(_eol==0||ch==_eol) { // skip CR after LF or LF after CR
				_eol=ch; // remember end-of-line character
				return str;
			}
		}
		else str.push_back(ch);
		_eol=0;
	}
}

std::string Uart::readAll() {
	const std::size_t increment=256;
	std::vector<char> buf(increment);
	std::size_t bytes=0;
	
	for(;;) {
		auto r=impl()->read(&buf[bytes],increment,0);
		if(r==0) break;
		bytes+=r;
		buf.resize(bytes+increment);
	}
	
	return std::string(buf.data(),bytes);
}

void Uart::setDTR(bool b) {
	impl()->setDTR(b);
}

bool Uart::getDSR() const {
	return impl()->getDSR();
}

void Uart::setRTS(bool b) {
	impl()->setRTS(b);
}

bool Uart::getCTS() const {
	return impl()->getCTS();
}

std::vector<std::string> Uart::listSerialPorts() {
	return UartImpl::listSerialPorts();
}

UartImpl *Uart::impl() {
	if(!_impl) throw std::runtime_error("Port is not opened");
	return _impl;
}

const UartImpl *Uart::impl() const {
	if(!_impl) throw std::runtime_error("Port is not opened");
	return _impl;
}
