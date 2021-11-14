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
 * Redirects standard output.
 */

#include "ioredirector.h"

#include <mutex>
#include <thread>
#include <atomic>
#include <iostream>
#include <cstdio>
#include <stdexcept>
#include <utility>

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

struct IORedirector::IORedirectorData {
#ifdef _WIN32
	HANDLE hStdoutRead {NULL};
	HANDLE hStdoutWrite {NULL};
	
	int startWin32Relaunch();
	int startWin32NoRelaunch();
#else
	int rpipe,wpipe;
#endif

	Mode _mode=Disabled;
	std::string queue;
	std::mutex m;
	std::thread t;
	std::function<void(const std::string &)> hook;
	std::atomic<bool> exitThread {false};
	std::atomic<bool> enabled {true};
};

#define PIPE_READ_BUF 1024

#ifdef _WIN32

#include <sstream>
#include <vector>

// on Windows, stdint.h is not always present, but intptr_t is defined in cstddef
#include <cstddef> 

#include <io.h>
#include <fcntl.h>

#define REDIRECTENVVAR L"SDM_IOREDIRECTOR_STDOUT_HANDLE"

int IORedirector::start(bool alt) {
	if(alt) {
		int r=startWin32NoRelaunch();
		data->_mode=Alternative;
		return r;
	}
	else {
		int r=startWin32Relaunch();
		if(r!=RequestExit) data->_mode=Normal;
		return r;
	}
}

int IORedirector::startWin32Relaunch() {
	BOOL b;
	DWORD dw;
	std::vector<wchar_t> moduleName(1024);
	wchar_t *env=NULL;
	std::wostringstream ssenv;
	std::vector<wchar_t> envNew;
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	
// Check whether we are the first instance
	dw=GetEnvironmentVariableW(REDIRECTENVVAR,NULL,0);
	
	if(!dw) {
// We are the first instance, relaunch itself with redirected stdout
// Create pipe with inheritable handles
		SECURITY_ATTRIBUTES pipe_sa;
		pipe_sa.nLength=sizeof(SECURITY_ATTRIBUTES);
		pipe_sa.lpSecurityDescriptor=NULL;
		pipe_sa.bInheritHandle=TRUE;
		
		HANDLE hReadPipe=NULL,hWritePipe=NULL;
		b=CreatePipe(&hReadPipe,&hWritePipe,&pipe_sa,0);
		if(!b) throw std::runtime_error("Cannot create pipe");
		
		try {
// Get current process file name
			for(;;) {
				dw=GetModuleFileNameW(NULL,&moduleName[0],static_cast<DWORD>(moduleName.size()));
				if(!dw) throw std::runtime_error("Cannot obtain current module file name");
				else if(dw==moduleName.size()) { // insufficient buffer, enlarge
					if(moduleName.size()>=32768) {
						throw std::runtime_error("Cannot obtain current module file name");
					}
					moduleName.resize(moduleName.size()*2);
				}
				else break;
			}

// Get current process environment block (terminated by two null characters)
			env=GetEnvironmentStringsW();
			if(!env) throw std::runtime_error("Can't get current process environment");
			int len=1;
			while(env[len]||env[len-1]) len++;
// Append read pipe handle to the environment
			ssenv<<std::wstring(env,len);
			ssenv<<REDIRECTENVVAR<<L"=";
			ssenv<<reinterpret_cast<uintptr_t>(hReadPipe)<<L' ';
			ssenv<<reinterpret_cast<uintptr_t>(hWritePipe);
			ssenv<<L'\0'<<L'\0';
			envNew.resize(ssenv.str().size(),0);
			memcpy(&envNew[0],ssenv.str().c_str(),ssenv.str().size()*sizeof(wchar_t));
			
// Populate STARTUPINFO structure
			ZeroMemory(&si,sizeof(STARTUPINFOW));
			si.cb=sizeof(STARTUPINFOW);
			si.dwFlags=STARTF_USESTDHANDLES;
			si.hStdInput=INVALID_HANDLE_VALUE; // don't redirect stdin
			si.hStdOutput=hWritePipe;
			si.hStdError=hWritePipe;
		}
		catch(std::exception &) {
			if(env) FreeEnvironmentStringsW(env);
			CloseHandle(hReadPipe);
			CloseHandle(hWritePipe);
			throw;
		}
		
// Relaunch self
		b=CreateProcessW(&moduleName[0],
			GetCommandLineW(), // repeat current process command line
			NULL,
			NULL,
			TRUE, // inherit handles
			CREATE_UNICODE_ENVIRONMENT,
			&envNew[0], // new environment containing pipe handle
			NULL,
			&si,
			&pi);
		
		FreeEnvironmentStringsW(env);
		CloseHandle(hReadPipe);
		CloseHandle(hWritePipe);
		
		if(!b) throw std::runtime_error("Cannot spawn process");
		
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		
		return RequestExit;
	}
	else {
// We are the second. Obtain pipe handle
// On amd64, uintptr_t can be too large for atoi() and even strtol()
		std::vector<wchar_t> redirectVar(dw);
		dw=GetEnvironmentVariableW(REDIRECTENVVAR,&redirectVar[0],dw);
		uintptr_t pipe_int;
		std::wistringstream iss(&redirectVar[0]);
		
		iss>>pipe_int;
		data->hStdoutRead=reinterpret_cast<HANDLE>(pipe_int);
		iss>>pipe_int;
		data->hStdoutWrite=reinterpret_cast<HANDLE>(pipe_int);
		
		setvbuf(stdout,NULL,_IONBF,0);
		setvbuf(stderr,NULL,_IONBF,0);
		std::cout.flush();
		std::cout.clear();
		std::cerr.flush();
		std::cerr.clear();
		
		data->t=std::thread(&IORedirector::threadProc,data);
		
		return 0;
	}
}

int IORedirector::startWin32NoRelaunch() {
	HANDLE hWritePipe;
	
// Redirect stdout on kernel32 level. One would expect this to work also for
// the runtime library stdout, but it doesn't.
	BOOL b=CreatePipe(&data->hStdoutRead,&hWritePipe,NULL,0);
	if(!b) throw std::runtime_error("Cannot create pipe");
	SetStdHandle(STD_OUTPUT_HANDLE,hWritePipe);
	SetStdHandle(STD_ERROR_HANDLE,hWritePipe);

// Redirect the so-called "POSIX" stdout
	int posix_fd=_open_osfhandle(reinterpret_cast<intptr_t>(hWritePipe),_O_TEXT);
	if(posix_fd==-1) throw std::runtime_error("Cannot redirect POSIX stdout");
	_dup2(posix_fd,_fileno(stdout));
	_dup2(posix_fd,_fileno(stderr));

// Redirect stdio
	FILE *fp=_fdopen(posix_fd,"w");
	if(!fp) throw std::runtime_error("Cannot redirect stdio");
// Note: the following ugly hack is not guaranteed to work
	*stdout=*fp;
	*stderr=*fp;
	setvbuf(stdout,NULL,_IONBF,0);
	setvbuf(stderr,NULL,_IONBF,0);

// Reset Standard C++ library streams. Should became synced with stdio
	std::cout.flush();
	std::cout.clear();
	std::cerr.flush();
	std::cerr.clear();
	
	data->hStdoutWrite=hWritePipe;
	
	data->t=std::thread(&IORedirector::threadProc,data);
	
	return 0;
}

std::string IORedirector::readPipe(IORedirectorData &d) {
	char buf[PIPE_READ_BUF];
	DWORD dwBytesRead;

	BOOL b=ReadFile(d.hStdoutRead,buf,PIPE_READ_BUF,&dwBytesRead,NULL);
	if(b) return std::string(buf,static_cast<std::size_t>(dwBytesRead));
	else return std::string();
}

void IORedirector::closePipe() {
	if(data->hStdoutWrite) CloseHandle(data->hStdoutWrite);
}

#else // not _WIN32

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int IORedirector::start(bool) {
	int pipefd[2];
	int r=pipe(pipefd);
	if(r!=0) throw std::runtime_error("Cannot create pipe");
	data->rpipe=pipefd[0];
	data->wpipe=pipefd[1];
	r=dup2(data->wpipe,1);
	if(r==-1) throw std::runtime_error("Cannot duplicate stdout handle");
	r=dup2(data->wpipe,2);
	if(r==-1) throw std::runtime_error("Cannot duplicate stderr handle");
	
	setvbuf(stdout,NULL,_IONBF,0);
	setvbuf(stderr,NULL,_IONBF,0);
	std::cout.flush();
	std::cout.clear();
	std::cerr.flush();
	std::cerr.clear();
	
	data->_mode=Normal;
	
	data->t=std::thread(&IORedirector::threadProc,data);
	
	return 0;
}

std::string IORedirector::readPipe(IORedirectorData &d) {
	char buf[PIPE_READ_BUF];

	ssize_t r=::read(d.rpipe,buf,PIPE_READ_BUF);
	if(r==-1) return std::string();
	return std::string(buf,r);
}

void IORedirector::closePipe() {
	close(data->wpipe);
	close(1);
	close(2);
}

#endif

IORedirector::IORedirector(): data(new IORedirectorData) {}

IORedirector::~IORedirector() {
	data->exitThread=true;
	closePipe();
	if(data->t.joinable()) data->t.detach();
}

void IORedirector::threadProc(std::shared_ptr<IORedirectorData> pd) {
	for(;;) {
		auto str=readPipe(*pd);
		if(pd->exitThread) return;
		if(!pd->enabled||str.empty()) continue;
		
		std::unique_lock<std::mutex> am(pd->m);
		try {
			if(pd->hook) pd->hook(str);
			else pd->queue+=str;
		}
		catch(std::exception &) {
			pd->queue.clear();
			pd->queue.shrink_to_fit();
		}
	}
}

std::string IORedirector::read() {
	std::unique_lock<std::mutex> am(data->m);
	std::string res;
	std::swap(res,data->queue);
	return res;
}

void IORedirector::setHook(const Hook &h) {
	std::unique_lock<std::mutex> am(data->m);
	data->hook=h;
	if(!data->hook) return;
	data->hook(data->queue);
	data->queue.clear();
}

IORedirector::Mode IORedirector::mode() const {
	return data->_mode;
}

void IORedirector::enable(bool b) {
	data->enabled=b;
}
