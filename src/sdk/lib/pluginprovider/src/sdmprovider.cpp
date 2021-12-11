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
