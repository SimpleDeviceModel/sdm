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
 * This header files defines plugin, device, channel and stream classes
 * for the Dcl plugin.
 */

#ifndef DCL_H_INCLUDED
#define DCL_H_INCLUDED

#include "sdmprovider.h"
#include "uart.h"

#include <cstdint>

typedef std::uint8_t Byte;

// Plugin class

class DclPlugin : public SDMAbstractPlugin {
public:
	DclPlugin();
	virtual SDMAbstractDevice *openDevice(int id) override;
};

// Device class

class DclDevice : public SDMAbstractDevice {
	Uart _port; // serial port used to communicate
public:
	DclDevice();
	
	virtual int close() override;
	
	virtual SDMAbstractChannel *openChannel(int id) override;
	
	virtual int connect() override;
	virtual int disconnect() override;
	virtual int getConnectionStatus() override;
};

// Channel class

class DclChannel : public SDMAbstractChannel {
	Uart &_port;
public:
	DclChannel(Uart &port);
	
	virtual int close() override;
	virtual int writeReg(sdm_addr_t addr,sdm_reg_t data) override;
	virtual sdm_reg_t readReg(sdm_addr_t addr,int *status) override;
	virtual int writeMem(sdm_addr_t addr,const sdm_reg_t *data,std::size_t n) override;
	virtual int readMem(sdm_addr_t addr,sdm_reg_t *data,std::size_t n) override;
private:
	void sendBytes(const std::vector<Byte> &s);
	std::vector<Byte> recvBytes(std::size_t n);
};

#endif
