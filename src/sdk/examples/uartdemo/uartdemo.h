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
 * This header files defines classes for the UartDemo plugin.
 */

#ifndef UARTDEMO_H_INCLUDED
#define UARTDEMO_H_INCLUDED

#include "sdmprovider.h"
#include "uart.h"

class UartPlugin : public SDMAbstractPluginProvider {
public:
	UartPlugin();
	virtual SDMAbstractDeviceProvider *openDevice(int id) override;
};

class UartDevice : public SDMAbstractDeviceProvider {
	Uart _port;
public:
	UartDevice();
	
	virtual int close() override;
	
	virtual SDMAbstractChannelProvider *openChannel(int id) override;
	virtual SDMAbstractSourceProvider *openSource(int id) override;
	
	virtual int connect() override;
	virtual int disconnect() override;
	virtual int getConnectionStatus() override;
};

class UartChannel : public SDMAbstractChannelProvider {
	Uart &_port;
public:
	UartChannel(Uart &port);
	
	virtual int close() override;
	
	virtual int writeReg(sdm_addr_t addr,sdm_reg_t data) override;
	virtual sdm_reg_t readReg(sdm_addr_t addr,int *status) override;
};
/*
class UartSource : public SDMAbstractSourceProvider {
	struct Streams {
		int npacket[2] {};
		int pos[2] {};
		bool selectedStreams[2] {};
		int df=1;
		std::chrono::steady_clock::time_point begin;
		
		sdm_sample_t word(int stream,int i);
	};
	
	int _id;
	Streams _s;
	const bool &_connected;
	int _msPerPacket=10;
public:
	UartSource(int id,const bool &connected);
	
	virtual int close() override;
	
	virtual int selectReadStreams(const int *streams,std::size_t n,std::size_t packets,int df) override;
	virtual int readStream(int stream,sdm_sample_t *data,std::size_t n,int nb) override;
	virtual int readNextPacket() override;
	virtual void discardPackets() override;
	virtual int readStreamErrors() override;
	
	virtual void setProperty(const std::string &name,const std::string &value) override;
};
*/

#endif
