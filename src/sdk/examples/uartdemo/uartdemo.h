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
 * This header files defines plugin, device, channel and stream classes
 * for the UartDemo plugin.
 */

#ifndef UARTDEMO_H_INCLUDED
#define UARTDEMO_H_INCLUDED

#include "sdmprovider.h"
#include "uart.h"

#include <deque>

// Plugin class

class UartPlugin : public SDMAbstractPlugin {
public:
	UartPlugin();
	virtual SDMAbstractDevice *openDevice(int id) override;
};

// Device class

class UartDevice : public SDMAbstractDevice {
	Uart _port; // serial port used to communicate
	std::deque<char> _q; // stream data buffer
public:
	UartDevice();
	
	virtual int close() override;
	
	virtual SDMAbstractChannel *openChannel(int id) override;
	virtual SDMAbstractSource *openSource(int id) override;
	
	virtual int connect() override;
	virtual int disconnect() override;
	virtual int getConnectionStatus() override;
};

// Channel class

class UartChannel : public SDMAbstractChannel {
	Uart &_port;
	std::deque<char> &_q;
public:
	UartChannel(Uart &port,std::deque<char> &q);
	
	virtual int close() override;
	
	virtual int writeReg(sdm_addr_t addr,sdm_reg_t data) override;
	virtual sdm_reg_t readReg(sdm_addr_t addr,int *status) override;
/*
 * Note: Default implementations of writeFIFO(), readFIFO(), writeMem()
 * and readMem() provided by SDMAbstractChannel work by repeatedly
 * calling writeReg and readReg. If the communication protocol supported
 * block transactions, these methods should have been overriden for better
 * performance.
 */
private:
	void sendBytes(const std::string &s);
};

// Source class

class UartSource : public SDMAbstractSource {
	Uart &_port;
	std::deque<char> &_q;
	std::size_t _cnt=0; // number of samples delivered for the current packet
	bool _selected=false;
public:
	UartSource(Uart &port,std::deque<char> &q);
	
	virtual int close() override;
	
	virtual int selectReadStreams(const int *streams,std::size_t n,std::size_t packets,int df) override;
	virtual int readStream(int stream,sdm_sample_t *data,std::size_t n,int nb) override;
	virtual int readNextPacket() override;
	virtual void discardPackets() override;
	
private:
	std::size_t loadFromQueue(sdm_sample_t *data,std::size_t n);
	bool endOfFrame() const;
};

#endif
