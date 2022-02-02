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
 * SDMAbstractQueuedSource members
 */

SDMAbstractQueuedSource::SDMAbstractQueuedSource():
	_errors(0) {}

int SDMAbstractQueuedSource::selectReadStreams(const int *streams,std::size_t n,std::size_t packets,int df) {
	_pos.clear();
	for(std::size_t i=0;i<n;i++) _pos[streams[i]]=0;
	clear();
	return 0;
}

int SDMAbstractQueuedSource::readStream(int stream,sdm_sample_t *data,std::size_t n,int nb) {
// Exit immediately if no data has been requested
	if(n==0) return 0;
	
// Return an error if the stream id has not been selected
	std::map<int,std::size_t>::iterator it=_pos.find(stream);
	if(it==_pos.end()) return SDM_ERROR;
	std::size_t &currentPos=it->second;
	
// Process data from the queue
	bool eop=false;
	std::size_t loaded=getSamplesFromQueue(stream,currentPos,data,n,eop);
	if(isError()) _errors++;
	currentPos+=loaded;
	
	for(;;) {
// Read new data from the device
		std::size_t suggestToRead=n-loaded;
		bool nonBlocking=false;
		if(nb!=0) nonBlocking=true; // don't block if non-blocking read is requested
		if(loaded>0) nonBlocking=true; // don't block if we have already produced some data
		if(eop) nonBlocking=true; // don't block if the packet has been already finished
		addDataToQueue(suggestToRead,nonBlocking);
// Put new samples to the buffer
		if(loaded<n&&!eop) {
			std::size_t r=getSamplesFromQueue(stream,currentPos,data+loaded,n-loaded,eop);
			if(isError()) _errors++;
			currentPos+=r;
			loaded+=r;
		}
// Break loop if there's no reason to block
		if(nb!=0) break;
		if(loaded>0) break;
		if(eop) break;
	}
	
	if(nb!=0&&loaded==0&&!eop) return SDM_WOULDBLOCK;
	return static_cast<int>(loaded);
}

int SDMAbstractQueuedSource::readNextPacket() {
	next();
	for(std::map<int,std::size_t>::iterator it=_pos.begin();it!=_pos.end();++it) it->second=0;
	return 0;
}

void SDMAbstractQueuedSource::discardPackets() {
	clear();
	for(std::map<int,std::size_t>::iterator it=_pos.begin();it!=_pos.end();++it) it->second=0;
	_errors=0;
}

int SDMAbstractQueuedSource::readStreamErrors() {
	return _errors;
}
