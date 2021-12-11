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
 * This module provides an implementation for the pluginprovider
 * library functions.
 */

#include "sdmprovider.h"

#include <climits>

/*
 * SDMAbstractPlugin members
 */ 

/*
 * SDMAbstractDevice members
 */

SDMAbstractChannel *SDMAbstractDevice::openChannel(int) {
	return NULL;
}

SDMAbstractSource *SDMAbstractDevice::openSource(int) {
	return NULL;
}

/*
 * SDMAbstractChannel members
 */

int SDMAbstractChannel::writeFIFO(sdm_addr_t addr,const sdm_reg_t *data,std::size_t n,int) {
	if(n>INT_MAX) n=INT_MAX;
	for(std::size_t i=0;i<n;i++) {
		int r=writeReg(addr,data[i]);
		if(r) return SDM_ERROR;
	}
	return static_cast<int>(n);
}

int SDMAbstractChannel::readFIFO(sdm_addr_t addr,sdm_reg_t *data,std::size_t n,int) {
	if(n>INT_MAX) n=INT_MAX;
	for(std::size_t i=0;i<n;i++) {
		int status;
		data[i]=readReg(addr,&status);
		if(status) return SDM_ERROR;
	}
	return static_cast<int>(n);
}

int SDMAbstractChannel::writeMem(sdm_addr_t addr,const sdm_reg_t *data,std::size_t n) {
	for(std::size_t i=0;i<n;i++) {
		int r=writeReg(addr++,data[i]);
		if(r) return SDM_ERROR;
	}
	return 0;
}

int SDMAbstractChannel::readMem(sdm_addr_t addr,sdm_reg_t *data,std::size_t n) {
	int status;
	for(std::size_t i=0;i<n;i++) {
		data[i]=readReg(addr++,&status);
		if(status) return SDM_ERROR;
	}
	return 0;
}

/*
 * SDMAbstractSource members
 */

int SDMAbstractSource::readStreamErrors() {
	return 0;
}

/*
 * SDMAbstractQueuedSource
 */

int SDMAbstractQueuedSource::selectReadStreams(const int *streams,std::size_t n,std::size_t packets,int df) {
	discardPackets();
	return 0;
}

int SDMAbstractQueuedSource::readStream(int stream,sdm_sample_t *data,std::size_t n,int nb) {
// Exit immediately if no data has been requested
	if(n==0) return 0;
	
// Process data from the queue
	auto loaded=getSamplesFromQueue(stream,data,n,_requestStartOfPacket);
	if(loaded>0) _requestStartOfPacket=false;
	
	for(;;) {
// Read new data from the device
		auto suggestToRead=n-loaded;
		bool nonBlocking=false;
		if(nb!=0) nonBlocking=true; // don't block if non-blocking read is requested
		if(loaded>0) nonBlocking=true; // don't block if we have already produced some data
		if(packetFinished()) nonBlocking=true; // don't block if the packet has been already finished
		addDataToQueue(suggestToRead,nonBlocking);
// Put new samples to the buffer
		if(loaded<n) loaded+=getSamplesFromQueue(stream,data+loaded,n-loaded,_requestStartOfPacket);
		if(loaded>0) _requestStartOfPacket=false;
// Break loop if there's no reason to block
		if(nb!=0) break;
		if(loaded>0) break;
		if(packetFinished()) break;
	}
	
	if(nb!=0&&loaded==0&&!packetFinished()) return SDM_WOULDBLOCK;
	return static_cast<int>(loaded);
}

int SDMAbstractQueuedSource::readNextPacket() {
	_requestStartOfPacket=true;
	return 0;
}

void SDMAbstractQueuedSource::discardPackets() {
	clear();
	_requestStartOfPacket=true;
	_errors=0;
}

int SDMAbstractQueuedSource::readStreamErrors() {
	return _errors;
}

bool SDMAbstractQueuedSource::packetFinished() const {
	return isStartOfPacket()&&!_requestStartOfPacket;
}
