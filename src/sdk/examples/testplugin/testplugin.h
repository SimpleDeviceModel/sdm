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
 * This header files defines classes for the test SDM plugin.
 */

#ifndef TESTPLUGIN_H_INCLUDED
#define TESTPLUGIN_H_INCLUDED

#include "sdmprovider.h"
#include "videoframe.h"

#include <deque>
#include <chrono>
#include <cstdint>

#ifdef _MSC_VER
// Disable useless warning about array initialization
	 #pragma warning(disable: 4351)
#endif

class TestPlugin : public SDMAbstractPlugin {
public:
	TestPlugin();
	virtual SDMAbstractDevice *openDevice(int id) override;
};

class TestDevice : public SDMAbstractDevice {
	bool _connected=false;
public:
	TestDevice(int id);
	
	virtual int close() override;
	
	virtual SDMAbstractChannel *openChannel(int id) override;
	virtual SDMAbstractSource *openSource(int id) override;
	
	virtual int connect() override;
	virtual int disconnect() override;
	virtual int getConnectionStatus() override;
};

class TestChannel : public SDMAbstractChannel {
	struct FifoItem {
		sdm_reg_t word;
		bool sop;
		FifoItem(): word(0),sop(false) {}
		FifoItem(sdm_reg_t w, bool b): word(w),sop(b) {}
	};
	
	int _id;
	sdm_reg_t _regs[256] {};
	std::deque<FifoItem> _fifo0;
	bool _fifo0_next=false;
	const bool &_connected;
public:
	TestChannel(int id,const bool &connected);
	
	virtual int close() override;
	
	virtual int writeReg(sdm_addr_t addr,sdm_reg_t data) override;
	virtual sdm_reg_t readReg(sdm_addr_t addr,int *status) override;
	virtual int writeFIFO(sdm_addr_t addr,const sdm_reg_t *data,std::size_t n,int flags) override;
	virtual int readFIFO(sdm_addr_t addr,sdm_reg_t *data,std::size_t n,int flags) override;
	virtual int writeMem(sdm_addr_t addr,const sdm_reg_t *data,std::size_t n) override;
	virtual int readMem(sdm_addr_t addr,sdm_reg_t *data,std::size_t n) override;
};

class TestSource : public SDMAbstractSource {
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
	TestSource(int id,const bool &connected);
	
	virtual int close() override;
	
	virtual int selectReadStreams(const int *streams,std::size_t n,std::size_t packets,int df) override;
	virtual int readStream(int stream,sdm_sample_t *data,std::size_t n,int nb) override;
	virtual int readNextPacket() override;
	virtual void discardPackets() override;
	virtual int readStreamErrors() override;
	
	virtual void setProperty(const std::string &name,const std::string &value) override;
};

class VideoSource : public SDMAbstractSource {
	const bool &_connected;
	
	bool _selected=false;
	std::chrono::steady_clock::time_point _begin;
	int _df=1;
	
	int _npacket=0;
	int _pos=0;
	int _msPerPacket=10;
	
	VideoFrame _frame;
	bool _simpleMode=false;
public:
	VideoSource(const bool &connected);
	
	virtual int close() override;
	
	virtual int selectReadStreams(const int *streams,std::size_t n,std::size_t packets,int df) override;
	virtual int readStream(int stream,sdm_sample_t *data,std::size_t n,int nb) override;
	virtual int readNextPacket() override;
	virtual void discardPackets() override;
	virtual int readStreamErrors() override;
	
	virtual void setProperty(const std::string &name,const std::string &value) override;
};

#endif
