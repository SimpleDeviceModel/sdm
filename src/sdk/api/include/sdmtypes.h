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
 * This header file defines some common SDM data types and constants
 * used by both plugin and client.
 */

#ifndef SDMTYPES_H_INCLUDED
#define SDMTYPES_H_INCLUDED

/********************************************************************
 * Calling convention
 *******************************************************************/

#if defined(__i386) || defined(_M_IX86)
	#if defined(__GNUC__) /* for GCC, regardless of target platform */
		#define SDMCALL __attribute__((__cdecl__))
	#elif defined(_WIN32)
		#define SDMCALL __cdecl
	#else
		#define SDMCALL
	#endif
#else
	#define SDMCALL
#endif

/********************************************************************
 * Export public API symbols
 *******************************************************************/

#ifdef __cplusplus
	#define SDM_LINKAGE extern "C"
#else
	#define SDM_LINKAGE
#endif

#ifdef EXPORT_SDM_SYMBOLS
	#ifdef _WIN32
		#define SDMAPI SDM_LINKAGE __declspec(dllexport)
	#elif (__GNUC__>=4)
		#define SDMAPI SDM_LINKAGE __attribute__((__visibility__("default")))
	#else
		#define SDMAPI SDM_LINKAGE
	#endif
#else
	#define SDMAPI SDM_LINKAGE
#endif

/********************************************************************
 * Data types
 *******************************************************************/

#if (__cplusplus>=199711L)
	#include <cstddef>
	using std::size_t;
#else
	#include <stddef.h>
#endif

#if (__cplusplus>=201103L) || (defined(__cplusplus) && (_MSC_VER>=1600))
	#include <cstdint>
	typedef std::uint32_t sdm_uint32_t;
#elif (__STDC_VERSION__>=199901L) || (_MSC_VER>=1600)
	#include <stdint.h>
	typedef uint32_t sdm_uint32_t;
#elif defined(__UINT32_TYPE__) /* usually predefined by GCC */
	typedef __UINT32_TYPE__ sdm_uint32_t;
#elif defined(_WIN32) && (defined(_MSC_VER) || defined(__BORLANDC__))
	typedef unsigned __int32 sdm_uint32_t;
#elif defined(_WIN32)
	#include <windef.h>
	typedef DWORD sdm_uint32_t;
#else
	#include <stdint.h>
	typedef uint32_t sdm_uint32_t;
#endif

typedef sdm_uint32_t sdm_addr_t;
typedef sdm_uint32_t sdm_reg_t;
typedef double sdm_sample_t;

/********************************************************************
 * Error codes
 *******************************************************************/

/* 
 * Note: SDM error codes are always negative.
 * Semantics for error codes from -1 to -999 are defined by the SDM
 * base system. Error codes from -1000 onwards can be used to report
 * additional error types defined by plugins.
 */

#define SDM_ERROR -1                /* Unspecified error */
#define SDM_WOULDBLOCK -2           /* Non-blocking operation request would block */

#define SDM_USERERROR -1000         /* User defined error */

/********************************************************************
 * Other constants
 *******************************************************************/

/* sdmWriteFIFO() / sdmReadFIFO() / sdmReadStream() flags */

#define SDM_FLAG_NONBLOCKING 1
#define SDM_FLAG_START 2
#define SDM_FLAG_NEXT 4

/********************************************************************
 * Function pointer types for import with LoadLibrary()/dlopen()
 *******************************************************************/

typedef int (SDMCALL *PtrSdmGetPluginProperty)(const char *,char *,size_t);
typedef int (SDMCALL *PtrSdmSetPluginProperty)(const char *,const char *);

typedef void * (SDMCALL *PtrSdmOpenDevice)(int);
typedef int (SDMCALL *PtrSdmCloseDevice)(void *);
typedef int (SDMCALL *PtrSdmGetDeviceProperty)(void *,const char *,char *,size_t);
typedef int (SDMCALL *PtrSdmSetDeviceProperty)(void *,const char *,const char *);
typedef int (SDMCALL *PtrSdmConnect)(void *);
typedef int (SDMCALL *PtrSdmDisconnect)(void *);
typedef int (SDMCALL *PtrSdmGetConnectionStatus)(void *);

typedef void * (SDMCALL *PtrSdmOpenChannel)(void *,int);
typedef int (SDMCALL *PtrSdmCloseChannel)(void *);
typedef int (SDMCALL *PtrSdmGetChannelProperty)(void *,const char *,char *,size_t);
typedef int (SDMCALL *PtrSdmSetChannelProperty)(void *,const char *,const char *);
typedef int (SDMCALL *PtrSdmWriteReg)(void *,sdm_addr_t,sdm_reg_t);
typedef sdm_reg_t (SDMCALL *PtrSdmReadReg)(void *,sdm_addr_t,int *);
typedef int (SDMCALL *PtrSdmWriteFIFO)(void *,sdm_addr_t,const sdm_reg_t *,size_t,int);
typedef int (SDMCALL *PtrSdmReadFIFO)(void *,sdm_addr_t,sdm_reg_t *,size_t,int);
typedef int (SDMCALL *PtrSdmWriteMem)(void *,sdm_addr_t,const sdm_reg_t *,size_t);
typedef int (SDMCALL *PtrSdmReadMem)(void *,sdm_addr_t,sdm_reg_t *,size_t);

typedef void * (SDMCALL *PtrSdmOpenSource)(void *,int);
typedef int (SDMCALL *PtrSdmCloseSource)(void *);
typedef int (SDMCALL *PtrSdmGetSourceProperty)(void *,const char *,char *,size_t);
typedef int (SDMCALL *PtrSdmSetSourceProperty)(void *,const char *,const char *);
typedef int (SDMCALL *PtrSdmSelectReadStreams)(void *,const int *,size_t,size_t,int);
typedef int (SDMCALL *PtrSdmReadStream)(void *,int,sdm_sample_t *,size_t,int);
typedef int (SDMCALL *PtrSdmReadNextPacket)(void *);
typedef void (SDMCALL *PtrSdmDiscardPackets)(void *);
typedef int (SDMCALL *PtrSdmReadStreamErrors)(void *);

#endif
