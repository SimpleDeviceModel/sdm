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
 * This header file defines the Uart class for cross-platform serial port
 * access. See also the "luart" section of the manual for the description
 * of semantics.
 */

#ifndef UART_H_INCLUDED
#define UART_H_INCLUDED

#include <vector>
#include <string>

class UartImpl;

class Uart {
public:
	enum StopBits {OneStop,TwoStops};
	enum Parity {NoParity,EvenParity,OddParity};
	enum FlowControl {NoFlowControl,HardwareFlowControl,SoftwareFlowControl};

private:
	UartImpl *_impl;
	char _eol;

public:
/*
 * The Uart object is movable, but not copyable. When it is destroyed,
 * the port is closed.
 */
	Uart();
	Uart(const Uart &)=delete;
	Uart(Uart &&orig);
	virtual ~Uart();
	
	Uart &operator=(const Uart &)=delete;
	Uart &operator=(Uart &&orig);
	
	void swap(Uart &other);
/*
 * Open/close port
 */
	operator bool() const; // return true if opened
	void open(const std::string &portName);
	void close();
/*
 * Configuration
 */
	int baudRate() const;
	void setBaudRate(int i);
	
	int dataBits() const;
	void setDataBits(int i);
	
	StopBits stopBits() const;
	void setStopBits(StopBits i);
	
	Parity parity() const;
	void setParity(Parity p);
	
	FlowControl flowControl() const;
	void setFlowControl(FlowControl f);
/*
 * Send/receive data
 * 
 * Note: "timeout" is measured in msecs, 0 indicates a non-blocking
 * operation, -1 waits indefinitely.
 * readLine() reads until the next newline character and is always
 * blocking. The newline character itself is not returned.
 * readAll() returns all data that are currently available for reading,
 * non-blocking.
 */
	std::size_t write(const char *buf,std::size_t n,int timeout=-1);
	std::size_t read(char *buf,std::size_t n,int timeout=-1);
	std::string readLine();
	std::string readAll();
/*
 * Individual signals
 */
	void setDTR(bool b);
	bool getDSR() const;
	
	void setRTS(bool b);
	bool getCTS() const;

/*
 * Static member functions
 */
	static std::vector<std::string> listSerialPorts();
	
/*
 * Private member functions
 */
private:
	UartImpl *impl();
	const UartImpl *impl() const;
};

#endif
