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
 * This header file defines a set of abstract classes which can be
 * subclassed to provide SDM plugin implementation.
 */

#ifndef SDMPROVIDER_H_INCLUDED
#define SDMPROVIDER_H_INCLUDED

#include "sdmproperty.h"
#include "sdmtypes.h"

#include <map>

class SDMAbstractDevice;
class SDMAbstractChannel;
class SDMAbstractSource;

class SDMAbstractPlugin : public SDMPropertyManager {
public:
	virtual ~SDMAbstractPlugin() {}
	
	virtual SDMAbstractDevice *openDevice(int id)=0;
	
// Note: instance() function must be defined by the user
	static SDMAbstractPlugin *instance();
};

class SDMAbstractDevice : public SDMPropertyManager {
public:
	virtual ~SDMAbstractDevice() {}
	
	virtual int close()=0;
	
	virtual SDMAbstractChannel *openChannel(int id) {return NULL;}
	virtual SDMAbstractSource *openSource(int id) {return NULL;}
	
	virtual int connect()=0;
	virtual int disconnect()=0;
	virtual int getConnectionStatus()=0;
};

/*
 * Note 1: default writeFIFO() and readFIFO() implementations work
 * by repeatedly calling writeReg() and readReg() respectively.
 * 
 * They operate under the following assumptions:
 * 
 * 1) writeReg() and readReg() will never block due to FIFO being
 *    full or empty,
 * 2) FIFO doesn't support the notion of packets.
 * 
 * If at least one of these assumptions is not true, these functions
 * must be overriden.
 *
 * Note 2: default implementations for writeMem() and readMem() work
 * by repeatedly calling writeReg() and readReg() respectively,
 * incrementing address each time.
 */

class SDMAbstractChannel : public SDMPropertyManager {
public:
	virtual ~SDMAbstractChannel() {}
	
	virtual int close()=0;
	
	virtual int writeReg(sdm_addr_t addr,sdm_reg_t data)=0;
	virtual sdm_reg_t readReg(sdm_addr_t addr,int *status)=0;
	virtual int writeFIFO(sdm_addr_t addr,const sdm_reg_t *data,std::size_t n,int flags);
	virtual int readFIFO(sdm_addr_t addr,sdm_reg_t *data,std::size_t n,int flags);
	virtual int writeMem(sdm_addr_t addr,const sdm_reg_t *data,std::size_t n);
	virtual int readMem(sdm_addr_t addr,sdm_reg_t *data,std::size_t n);
};

class SDMAbstractSource : public SDMPropertyManager {
public:
	virtual ~SDMAbstractSource() {}
	
	virtual int close()=0;
	
	virtual int selectReadStreams(const int *streams,std::size_t n,std::size_t packets,int df)=0;
	virtual int readStream(int stream,sdm_sample_t *data,std::size_t n,int nb)=0;
	virtual int readNextPacket()=0;
	virtual void discardPackets()=0;
	virtual int readStreamErrors() {return 0;}
};

/*
 * When deriving source classes from SDMAbstractSource, implementing
 * readStream() can be tricky since it is necessary to account for many
 * conditions (start/continuation of packet, blocking/non-blocking etc.)
 * which leads to mistakes. The SDMAbstractQueuedSource class is designed
 * to make implementing source classes easier by separating communication
 * with the device and packet queue management. Its interface is not
 * considered stable yet.
 * 
 * Derivatives of this class should implement some kind of queue to store
 * unprocessed data from the device.
 * 
 * addDataToQueue() reads some data from the device and adds them to the
 * queue. "samples" can be used as a hint on how much data to request, but
 * can be ignored. If "nonBlocking" is false, the function must not return
 * until it read at least 1 byte from the device. If the queue is currently
 * empty, addDataToQueue() should start adding data from the start of the
 * packet.
 * 
 * getSamplesFromQueue() fills the provided buffer ("data") with up to "n"
 * samples from the current packet in the queue, starting with "pos".
 * Returns the number of samples added to the buffer. If no samples have been
 * returned because of the end of the current packet (pos >= size of
 * the current packet), "eop" must be set to true.
 * 
 * next() removes the current packet from queue. This affects all streams.
 * 
 * clear() clears the queue. It should also try to clear data stored in
 * OS and hardware buffers if possible. This affects all streams.
 * 
 * isError() returns true when the last call to getSamplesFromQueue() resulted
 * in broken stream continuity.
 * 
 */

class SDMAbstractQueuedSource : public SDMAbstractSource {
	std::map<int,std::size_t> _pos;
	int _errors;
public:
	SDMAbstractQueuedSource();
	virtual int selectReadStreams(const int *streams,std::size_t n,std::size_t packets,int df);
	virtual int readStream(int stream,sdm_sample_t *data,std::size_t n,int nb);
	virtual int readNextPacket();
	virtual void discardPackets();
	virtual int readStreamErrors();
protected:
	virtual void addDataToQueue(std::size_t samples,bool nonBlocking)=0;
	virtual std::size_t getSamplesFromQueue(int stream,std::size_t pos,sdm_sample_t *data,std::size_t n,bool &eop)=0;
	virtual void next()=0;
	virtual void clear()=0;
	virtual bool isError() const {return false;}
};

/*
 * The following typedefs are deprecated and provided to support old code.
 */

typedef SDMAbstractPlugin SDMAbstractPluginProvider;
typedef SDMAbstractDevice SDMAbstractDeviceProvider;
typedef SDMAbstractChannel SDMAbstractChannelProvider;
typedef SDMAbstractSource SDMAbstractSourceProvider;

#endif
