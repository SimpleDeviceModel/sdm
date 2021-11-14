/*
 * Copyright (c) 2015-2021 by Microproject LLC
 * 
 * This file is part of the Simple Device Model (SDM) framework.
 * 
 * SDM framework is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SDM framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SDM framework.  If not, see <https://www.gnu.org/licenses/>.
 *
 * U8E is a library facilitating UTF-8 use in cross-platfrom
 * applications. Assuming that an application uses UTF-8 to represent
 * all text, U8E handles interaction with the environment, ensuring
 * that standard streams, file names, environment variables and
 * command line arguments are encoded properly. U8E implements text
 * codec classes which can be also used directly if needed.
 *
 * On Windows U8E works by using the wide-character versions of
 * API functions. On other platforms it uses locale-dependent
 * multibyte encoding (as reported by nl_langinfo) to interact with
 * the environment.
 *
 * See the corresponding header file for details.
 */

#include "u8eenv.h"
#include "u8ecodec.h"

#include <stdexcept>

#ifdef _WIN32

#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

namespace u8e {
	std::vector<std::string> cmdArgs(int,char**,Encoding enc) {
		int argc;
		std::vector<std::string> args;
		wchar_t **wargs=CommandLineToArgvW(GetCommandLineW(),&argc);
		if(!wargs||!argc) return args;
		try {
			WCodec codec(enc);
			for(int i=0;i<argc;i++) {
				args.push_back(codec.transcode(wargs[i]));
				codec.reset();
			}
		}
		catch(...) {
			LocalFree(wargs);
			throw;
		}
		LocalFree(wargs);
		return args;
	}
	
	std::string envVar(const std::string &name,Encoding enc) {
		WCodec codec(enc);
		std::wstring wname=codec.transcode(name);
		
		DWORD dw=GetEnvironmentVariableW(wname.c_str(),NULL,0);
		if(dw==0) return std::string();
		
		std::vector<wchar_t> wbuf(dw);
		dw=GetEnvironmentVariableW(wname.c_str(),&wbuf[0],dw);
		if(dw==0) return std::string();
		return codec.transcode(&wbuf[0]);
	}
	
	void setEnvVar(const std::string &name,const std::string &value,Encoding enc) {
		WCodec codec(enc);
		std::wstring localName=codec.transcode(name);
		codec.reset();
		std::wstring localValue=codec.transcode(value);
		
		BOOL b=SetEnvironmentVariableW(localName.c_str(),localValue.c_str());
		if(!b) throw std::runtime_error("Can't set environment variable");
	}
	
	void delEnvVar(const std::string &name,Encoding enc) {
		WCodec codec(enc);
		BOOL b=SetEnvironmentVariableW(codec.transcode(name).c_str(),NULL);
		if(!b) throw std::runtime_error("Can't delete environment variable");
	}
}

#else // not Windows

#include <cstdlib>

namespace u8e {
	std::vector<std::string> cmdArgs(int argc,char *argv[],Encoding enc) {
		std::vector<std::string> args;
		Codec codec(LocalMB,enc);
		for(int i=0;i<argc;i++) {
			args.push_back(codec.transcode(argv[i]));
			codec.reset();
		}
		return args;
	}
	
	std::string envVar(const std::string &name,Encoding enc) {
		Codec toLocal(enc,LocalMB);
		Codec fromLocal(LocalMB,enc);
		char *var=getenv(toLocal.transcode(name).c_str());
		if(!var) return std::string();
		return fromLocal.transcode(var);
	}
	
	void setEnvVar(const std::string &name,const std::string &value,Encoding enc) {
		Codec toLocal(enc,LocalMB);
		std::string localName=toLocal.transcode(name);
		toLocal.reset();
		std::string localValue=toLocal.transcode(value);
		int r=setenv(localName.c_str(),localValue.c_str(),1);
		if(r==-1) throw std::runtime_error("Can't set environment variable");
	}
	
	void delEnvVar(const std::string &name,Encoding enc) {
		Codec toLocal(enc,LocalMB);
		int r=unsetenv(toLocal.transcode(name).c_str());
		if(r==-1) throw std::runtime_error("Can't delete environment variable");
	}
}

#endif
