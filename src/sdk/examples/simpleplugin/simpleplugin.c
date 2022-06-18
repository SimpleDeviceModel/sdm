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
 * This module implements a simple SDM plugin example. More information
 * on plugin development is available in the manual.
 */

/* The following macro ensures that SDM API functions are exported */

#define EXPORT_SDM_SYMBOLS

/* Suppress warnings about strcpy and sprintf when using Visual Studio */

#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "sdmapi.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

/********************************************************************
 * Data structures to store object states
 *******************************************************************/

struct Device {
	char address[256];
	int connected;
};

struct Channel {
	char name[256];
	sdm_reg_t regs[256];
};

struct Source {
	char name[256];
	int selected;
	size_t pos;
	size_t packetSize;
	time_t t;
};

/* Macros to cast void* to Device/Channel/Source */

#define DEV(x) (*(struct Device *)(x))
#define CH(x) (*(struct Channel *)(x))
#define SRC(x) (*(struct Source *)(x))

/********************************************************************
 * Internally used functions
 *******************************************************************/

/*
 * A helper function to process property requests.
 * Used by sdmGet*Property exported functions.
 * Copies the property value to the supplied buffer if it is large enough,
 * otherwise returns the required buffer size.
 */

static int returnProperty(char *buf,size_t n,const char *value) {
	size_t size=strlen(value)+1; /* plus terminating null character */
	if(!size||size>INT_MAX) return SDM_ERROR;
	if(n<size) return (int)size;
	memcpy(buf,value,size);
	return 0;
}

/********************************************************************
 * Exported plugin functions
 *******************************************************************/

/*
 * Get/set plugin properties. Notable plugin properties include:
 *    "Name" - plugin name
 *    "Devices" - list of device names, ordered by id
 * 
 * Predefined property lists:
 *    "*" - list of all properties
 *    "*ro" - list of read-only properties
 *    "*wr" - list of writable properties
 * 
 * All lists are in CSV format as defined in RFC 4180.
 */

SDMAPI int SDMCALL sdmGetPluginProperty(const char *name,char *buf,size_t n) {
	const char *property;
	
	if(!strcmp(name,"Name")) property="Simple plugin example";
	else if(!strcmp(name,"Vendor")) property="Simple plugin vendor";
	else if(!strcmp(name,"Devices")) property="Example device";
	else if(!strcmp(name,"*")) property="Name,Vendor,Devices";
	else if(!strcmp(name,"*ro")) property="Name,Vendor,Devices";
	else return SDM_ERROR;
	
	return returnProperty(buf,n,property);
}

SDMAPI int SDMCALL sdmSetPluginProperty(const char *name,const char *value) {
	(void)name; /* property name - ignored */
	(void)value; /* property value - ignored */
	
	return SDM_ERROR; /* no writable properties */
}

/********************************************************************
 * Exported device functions
 * 
 * Device objects manage connection and create channel and source
 * objects.
 *******************************************************************/

/* Create a new device object */

SDMAPI void * SDMCALL sdmOpenDevice(int id) {
	struct Device *dev;
	
	if(id!=0) return NULL;
	dev=calloc(1,sizeof(struct Device));
	if(!dev) return NULL;
	strcpy(dev->address,"192.168.0.10");
	return dev;
}

/* Close the device object */

SDMAPI int SDMCALL sdmCloseDevice(void *h) {
	free(h);
	return 0;
}

/*
 * Get/set device properties. Notable device properties include:
 *    "Name" - device name
 *    "Channels" - list of channel names, ordered by id
 *    "Sources" - list of source names, ordered by id
 *    "ConnectionParameters" - list of properties used as connection
 * parameters
 * 
 * Predefined property lists - as for plugin properties.
 */

SDMAPI int SDMCALL sdmGetDeviceProperty(void *h,const char *name,char *buf,size_t n) {
	const char *property;
	
	if(!strcmp(name,"Name")) property="Example device";
	else if(!strcmp(name,"Address")) property=DEV(h).address;
	else if(!strcmp(name,"AutoOpenChannels")) property="open";
	else if(!strcmp(name,"AutoOpenSources")) property="open";
	else if(!strcmp(name,"Channels")) property="Channel 1,Channel 2";
	else if(!strcmp(name,"Sources")) property="Source 1,Source 2";
	else if(!strcmp(name,"ConnectionParameters")) property="Address";
	else if(!strcmp(name,"*")) property="Name,Address,AutoOpenChannels,AutoOpenSources,Channels,Sources,ConnectionParameters";
	else if(!strcmp(name,"*ro")) property="Name,AutoOpenChannels,AutoOpenSources,Channels,Sources,ConnectionParameters";
	else if(!strcmp(name,"*wr")) property="Address";
	else return SDM_ERROR;
	
	return returnProperty(buf,n,property);
}

SDMAPI int SDMCALL sdmSetDeviceProperty(void *h,const char *name,const char *value) {
	if(!strcmp(name,"Address")&&strlen(value)<256) {
		strcpy(DEV(h).address,value);
		return 0;
	}
	return SDM_ERROR;
}

/* Connect to the device. Parameters, if any, are specified using properties */

SDMAPI int SDMCALL sdmConnect(void *h) {
	if(DEV(h).connected) return SDM_ERROR;
	printf("Connecting with address %s\n",DEV(h).address);
	fflush(stdout); /* prevent buffering of redirected stdout on Win32 */
	DEV(h).connected=1;
	return 0;
}

/* Disconnect */

SDMAPI int SDMCALL sdmDisconnect(void *h) {
	if(!DEV(h).connected) return SDM_ERROR;
	printf("Closing connection\n");
	fflush(stdout); /* prevent buffering of redirected stdout on Win32 */
	DEV(h).connected=0;
	return 0;
}

/* Get connection status (non-zero means connected) */

SDMAPI int SDMCALL sdmGetConnectionStatus(void *h) {
	return DEV(h).connected;
}

/********************************************************************
 * Exported channel functions
 * 
 * Channel objects provide access to the device's address space.
 *******************************************************************/

/* Create a new channel object */

SDMAPI void * SDMCALL sdmOpenChannel(void *hdev,int id) {
	struct Channel *channel;

	(void)hdev; /* device handle - ignored */
	
	if(id!=0&&id!=1) return NULL;
	channel=calloc(1,sizeof(struct Channel));
	if(!channel) return NULL;
	if(id==0) strcpy(channel->name,"Channel 1");
	else strcpy(channel->name,"Channel 2");
	return channel;
}

/* Close the channel object */

SDMAPI int SDMCALL sdmCloseChannel(void *h) {
	free(h);
	return 0;
}

/*
 * Get/set channel properties. Notable channel properties include:
 *    "Name" - channel name
 * 
 * Predefined property lists - as for plugin properties.
 */

SDMAPI int SDMCALL sdmGetChannelProperty(void *h,const char *name,char *buf,size_t n) {
	const char *property;
	
	if(!strcmp(name,"Name")) property=CH(h).name;
	else if(!strcmp(name,"*")) property="Name";
	else if(!strcmp(name,"*ro")) property="Name";
	else return SDM_ERROR;
	
	return returnProperty(buf,n,property);
}

SDMAPI int SDMCALL sdmSetChannelProperty(void *h,const char *name,const char *value) {
	(void)h; /* channel handle - ignored */
	(void)name; /* property name - ignored */
	(void)value; /* property value - ignored */
	
	return SDM_ERROR; /* no writable properties */
}

/* Write data to a register */

SDMAPI int SDMCALL sdmWriteReg(void *h,sdm_addr_t addr,sdm_reg_t data) {
	if(addr>=256) return SDM_ERROR;
	CH(h).regs[addr]=data;
	return 0;
}

/* Read data from a register */

SDMAPI sdm_reg_t SDMCALL sdmReadReg(void *h,sdm_addr_t addr,int *status) {
	if(addr>=256) {
		if(status) *status=SDM_ERROR;
		return 0;
	}
	if(status) *status=0;
	return CH(h).regs[addr];
}

/* Write data to a FIFO (all words are written to a single address) */

SDMAPI int SDMCALL sdmWriteFIFO(void *h,sdm_addr_t addr,const sdm_reg_t *data,size_t n,int flags) {
	int r;
	size_t i;

	(void)flags; /* flags - ignored */
	
	if(n>INT_MAX) n=INT_MAX; /* it is allowed to write less than n words */
	
/* Note: to simplify things, we emulate a FIFO that never blocks
   and doesn't support packets. */
	
	for(i=0;i<n;i++) {
		r=sdmWriteReg(h,addr,data[i]);
		if(r<0) return r;
	}
	return (int)n;
}

/* Read data from a FIFO (all words are read from a single address) */

SDMAPI int SDMCALL sdmReadFIFO(void *h,sdm_addr_t addr,sdm_reg_t *data,size_t n,int flags) {
	int status;
	size_t i;

	(void)flags; /* flags - ignored */
	
	if(n>INT_MAX) n=INT_MAX; /* it is allowed to read less than n words */
	
/* Note: to simplify things, we emulate a FIFO that never blocks
   and doesn't support packets. */
	
	for(i=0;i<n;i++) {
		data[i]=sdmReadReg(h,addr,&status);
		if(status<0) return status;
	}
	return (int)n;
}

/* Write data to memory (address is incremented automatically) */

SDMAPI int SDMCALL sdmWriteMem(void *h,sdm_addr_t addr,const sdm_reg_t *data,size_t n) {
	if(addr+n>256) return SDM_ERROR;
	memcpy(&CH(h).regs[addr],data,n*sizeof(sdm_reg_t));
	return 0;
}

/* Read data from memory (address is incremented automatically) */

SDMAPI int SDMCALL sdmReadMem(void *h,sdm_addr_t addr,sdm_reg_t *data,size_t n) {
	if(addr+n>256) return SDM_ERROR;
	memcpy(data,&CH(h).regs[addr],n*sizeof(sdm_reg_t));
	return 0;
}

/********************************************************************
 * Exported source functions
 * 
 * Source objects capture data streams from the device.
 *******************************************************************/

/* Create a new source object */

SDMAPI void * SDMCALL sdmOpenSource(void *hdev,int id) {
	struct Source *src;

	(void)hdev; /* device handle - ignored */
	
	if(id!=0&&id!=1) return NULL;
	src=calloc(1,sizeof(struct Source));
	if(!src) return NULL;
	if(id==0) strcpy(src->name,"Source 1");
	else strcpy(src->name,"Source 2");
	src->packetSize=1000;
	src->t=time(NULL);
	return src;
}

/* Close the source object */

SDMAPI int SDMCALL sdmCloseSource(void *h) {
	free(h);
	return 0;
}

/*
 * Get/set source properties. Notable source properties include:
 *    "Name" - source name
 *    "Streams" - list of data stream names, ordered by id
 * 
 * Predefined property lists - as for plugin properties.
 */

SDMAPI int SDMCALL sdmGetSourceProperty(void *h,const char *name,char *buf,size_t n) {
	const char *property;
	char packetSizeBuf[256];
	
	if(!strcmp(name,"Name")) property=SRC(h).name;
	else if(!strcmp(name,"Streams")) property="Stream 1";
	else if(!strcmp(name,"PacketSize")) {
		sprintf(packetSizeBuf,"%u",(unsigned int)SRC(h).packetSize);
		property=packetSizeBuf;
	}
	else if(!strcmp(name,"*")) property="Name,Streams,PacketSize";
	else if(!strcmp(name,"*ro")) property="Name,Streams";
	else if(!strcmp(name,"*wr")) property="PacketSize";
	else return SDM_ERROR;
	
	return returnProperty(buf,n,property);
}

SDMAPI int SDMCALL sdmSetSourceProperty(void *h,const char *name,const char *value) {
	size_t packetSize;
	char *endptr;
	
	if(!strcmp(name,"PacketSize")) {
		packetSize=(size_t)strtoul(value,&endptr,0);
		if(endptr==value||packetSize==0||packetSize>65536) return SDM_ERROR;
		SRC(h).packetSize=packetSize;
		SRC(h).pos=0;
		return 0;
	}
	else return SDM_ERROR;
}

/* Select streams to read synchronously */

SDMAPI int SDMCALL sdmSelectReadStreams(void *h,const int *streams,size_t n,size_t packets,int df) {
	(void)packets; /* suggested number of packets to store - ignored */
	(void)df; /* decimation factor - ignored */
	
	if(n==0) { /* n==0 is used to deselect any selected streams */
		SRC(h).selected=0;
		return 0;
	}
	if(n>1) return SDM_ERROR; /* we have only one stream */
	if(*streams!=0) return SDM_ERROR; /* the only stream has ID 0 */
	SRC(h).selected=1;
	SRC(h).pos=0;
	return 0;
}

/* Read stream data. For synchronous reading, a stream must be selected first */

SDMAPI int SDMCALL sdmReadStream(void *h,int stream,sdm_sample_t *data,size_t n,int nb) {
	size_t i;
	size_t samples=n;
	time_t t;

	if(!SRC(h).selected) return SDM_ERROR; /* the stream must be selected first */
	if(stream!=0) return SDM_ERROR; /* the only stream we provide has ID 0 */
	if(n==0) return 0;
	if(n>INT_MAX) n=INT_MAX; /* it is allowed to read less than n samples */

/* 
 * Note: to avoid using platform-specific code in this example, we
 * don't emulate waiting for data in blocking mode. See testplugin.cpp
 * for a more realistic data stream emulation.
 */

/* Check for end-of-packet */
	if(SRC(h).pos>=SRC(h).packetSize) return 0;

/* In non-blocking mode, return SDM_WOULDBLOCK if at least 1 s of
   calendar time has not passed */
	t=time(NULL);
	if(nb&&difftime(t,SRC(h).t)<1) return SDM_WOULDBLOCK;
	SRC(h).t=t;
	
/* Return data from the current position. Return zero when packet ends.
 * Note that pos<packetSize if we reached this point */
	if(samples>SRC(h).packetSize-SRC(h).pos) samples=SRC(h).packetSize-SRC(h).pos;
	for(i=0;i<samples;i++) data[i]=rand();
	SRC(h).pos+=samples;
	return (int)samples;
}

/* Proceed to the next packet when reading synchronously */

SDMAPI int SDMCALL sdmReadNextPacket(void *h) {
	SRC(h).pos=0;
	return 0;
}

/* Discard all buffered packets */

SDMAPI void SDMCALL sdmDiscardPackets(void *h) {
	SRC(h).pos=0;
}

/* Number of stream errors since the last call to sdmSelectReadStreams()
   or sdmDiscardPackets() */

SDMAPI int SDMCALL sdmReadStreamErrors(void *h) {
	(void)h; /* source handle - ignored */
	return 0;
}
