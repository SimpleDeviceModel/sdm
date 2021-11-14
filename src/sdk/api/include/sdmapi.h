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
 * This header file declares function prototypes that must be
 * exported by an SDM plugin. When developing a plugin, define
 * EXPORT_SDM_SYMBOLS macro before including this file, or use an
 * appropriate compiler option.
 *
 * For greater compatibility, all exported functions are strictly
 * C-based without any C++ features.
 *
 * See the SDM manual for detailed function descriptions.
 */

#ifndef SDMAPI_H_INCLUDED
#define SDMAPI_H_INCLUDED

#include "sdmtypes.h"

/********************************************************************
 * Plugin functions
 *******************************************************************/
 
SDMAPI int SDMCALL sdmGetPluginProperty(const char *name,char *buf,size_t n);
SDMAPI int SDMCALL sdmSetPluginProperty(const char *name,const char *value);

/********************************************************************
 * Device functions
 *******************************************************************/

SDMAPI void * SDMCALL sdmOpenDevice(int id);
SDMAPI int SDMCALL sdmCloseDevice(void *h);
SDMAPI int SDMCALL sdmGetDeviceProperty(void *h,const char *name,char *buf,size_t n);
SDMAPI int SDMCALL sdmSetDeviceProperty(void *h,const char *name,const char *value);
SDMAPI int SDMCALL sdmConnect(void *h);
SDMAPI int SDMCALL sdmDisconnect(void *h);
SDMAPI int SDMCALL sdmGetConnectionStatus(void *h);

/********************************************************************
 * Control channel functions
 *******************************************************************/

SDMAPI void * SDMCALL sdmOpenChannel(void *hdev,int id);
SDMAPI int SDMCALL sdmCloseChannel(void *h);
SDMAPI int SDMCALL sdmGetChannelProperty(void *h,const char *name,char *buf,size_t n);
SDMAPI int SDMCALL sdmSetChannelProperty(void *h,const char *name,const char *value);
SDMAPI int SDMCALL sdmWriteReg(void *h,sdm_addr_t addr,sdm_reg_t data);
SDMAPI sdm_reg_t SDMCALL sdmReadReg(void *h,sdm_addr_t addr,int *status);
SDMAPI int SDMCALL sdmWriteFIFO(void *h,sdm_addr_t addr,const sdm_reg_t *data,size_t n,int flags);
SDMAPI int SDMCALL sdmReadFIFO(void *h,sdm_addr_t addr,sdm_reg_t *data,size_t n,int flags);
SDMAPI int SDMCALL sdmWriteMem(void *h,sdm_addr_t addr,const sdm_reg_t *data,size_t n);
SDMAPI int SDMCALL sdmReadMem(void *h,sdm_addr_t addr,sdm_reg_t *data,size_t n);

/********************************************************************
 * Data source functions
 *******************************************************************/

SDMAPI void * SDMCALL sdmOpenSource(void *hdev,int id);
SDMAPI int SDMCALL sdmCloseSource(void *h);
SDMAPI int SDMCALL sdmGetSourceProperty(void *h,const char *name,char *buf,size_t n);
SDMAPI int SDMCALL sdmSetSourceProperty(void *h,const char *name,const char *value);
SDMAPI int SDMCALL sdmSelectReadStreams(void *h,const int *streams,size_t n,size_t packets,int df);
SDMAPI int SDMCALL sdmReadStream(void *h,int stream,sdm_sample_t *data,size_t n,int nb);
SDMAPI int SDMCALL sdmReadNextPacket(void *h);
SDMAPI void SDMCALL sdmDiscardPackets(void *h);
SDMAPI int SDMCALL sdmReadStreamErrors(void *h);

#endif
