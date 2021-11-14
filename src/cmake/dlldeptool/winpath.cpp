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
 * DllDepTool is a tool for Windows deployment automation.
 * It scans the executable modules (EXE/DLL) and copies all the
 * DLLs required to run them.
 */

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "u8ecodec.h"
#include "u8eenv.h"

#include "winpath.h"

#include <iostream>
#include <cctype>
#include <sstream>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

std::string strtolower(const std::string &str) {
	u8e::WCodec codec(u8e::UTF8);
	std::wstring wstr=codec.transcode(str);
	std::vector<wchar_t> wbuf(wstr.begin(),wstr.end());
	wbuf.push_back(0);
	CharLowerW(&wbuf[0]);
	return codec.transcode(&wbuf[0]);
}

std::string cleanupString(const std::string &str) {
	std::string res=str;
	std::size_t i;
	
// remove whitespace at the beginning and at the end
	i=res.find_first_not_of(" \t\x0D\x0A");
	if(i==std::string::npos) return std::string();
	if(i>0) res.erase(0,i);
	i=res.find_last_not_of(" \t\x0D\x0A");
	if(i==std::string::npos) return std::string();
	if(i<res.size()-1) res.erase(i+1); // res.size() is at least 1 here

// remove quotes (if any)
	if(!res.empty()) if(res[0]=='\"') res.erase(0,1);
	if(!res.empty()) if(res.back()=='\"') res.pop_back();
	return res;
}

std::string normalizePath(const std::string &strPath) {
// Cleanup
	std::string str=cleanupString(strPath);
	
// Try to compute path using Windows API
	u8e::WCodec codec(u8e::UTF8);
	std::wstring wstr=codec.transcode(str);
	DWORD dw=GetFullPathNameW(wstr.c_str(),0,NULL,NULL);
	if(dw) {
		std::vector<wchar_t> wbuf(dw);
		dw=GetFullPathNameW(wstr.c_str(),dw,&wbuf[0],NULL);
		if(dw) str=codec.transcode(&wbuf[0]);
	}
	
	if(str.empty()) return str;
	
	str=strtolower(str);
	
// Convert characters
	for(std::size_t i=0;i<str.size();i++) if(str[i]=='/') str[i]='\\';
	
	str=cleanupString(str);
	
	if(!str.empty()) if(str.back()=='\\') str.pop_back();
	
	return str;
}

void splitPath(const std::string &strPath,std::string &strDir,std::string &strFile) {
	if(strPath.empty()) {
		strDir="";
		strFile="";
	}
	
	std::string str=normalizePath(strPath);
	
	u8e::WCodec codec(u8e::UTF8);
	
// Try to use FindFirstFile to obtain correctly cased file name
	WIN32_FIND_DATAW wfd;
	HANDLE hFind=FindFirstFileW(codec.transcode(str).c_str(),&wfd);
	
	std::size_t i=str.find_last_of("\\");
	if(i==std::string::npos) {
		strDir=".";
		if(hFind!=INVALID_HANDLE_VALUE) strFile=codec.transcode(wfd.cFileName);
		else strFile=str;
	}
	else {
		strDir=str.substr(0,i);
		if(hFind!=INVALID_HANDLE_VALUE) strFile=codec.transcode(wfd.cFileName);
		else strFile=str.substr(i+1);
	}
	
	FindClose(hFind);
}

std::vector<std::string> getEnvPath() {
	std::vector<std::string> dirs;
	std::istringstream iss(u8e::envVar("PATH"));
	
	for(;;) {
		std::string dir;
		while(iss.peek()==' ') iss.ignore();
		if(iss.eof()) break;
		if(iss.peek()=='\"') {
			std::getline(iss,dir,'\"');
			while(iss.peek()==';') iss.ignore();
		}
		else std::getline(iss,dir,';');
		
		dir=normalizePath(dir);
		
		if(!dir.empty()) dirs.push_back(dir);
	}
	
	return dirs;
}

std::vector<std::string> getSysPath(PEType t) {
	std::vector<std::string> dirs;
	wchar_t wbuf[MAX_PATH+1];
	u8e::WCodec codec(u8e::UTF8);
	
	GetWindowsDirectoryW(wbuf,MAX_PATH);
	std::string strWinDir=normalizePath(codec.transcode(wbuf));
	
	if(sizeof(void*)==4) { // we are 32-bit
		if(t==pe32) dirs.push_back(strWinDir+"\\system32");
		else dirs.push_back(strWinDir+"\\sysnative");
	}
	else { // we are 64-bit
		if(t==pe32) dirs.push_back(strWinDir+"\\syswow64");
		else dirs.push_back(strWinDir+"\\system32");
	}
	
	dirs.push_back(strWinDir);
	
	return dirs;
}

std::vector<std::string> processWildcards(const std::string &strArgument) {
	std::vector<std::string> result;
	
	std::string strDir,strFile;
	
	splitPath(strArgument,strDir,strFile);
	
	u8e::WCodec codec(u8e::UTF8);
	WIN32_FIND_DATAW wfd;
	HANDLE hFind=FindFirstFileW(codec.transcode(strArgument).c_str(),&wfd);
	
	if(hFind==INVALID_HANDLE_VALUE) {
		result.push_back(strArgument);
		return result;
	}
	
	for(;;) {
		result.push_back(strDir+"\\"+codec.transcode(wfd.cFileName));
		codec.reset();
		BOOL b=FindNextFileW(hFind,&wfd);
		if(!b) break;
	}
	
	FindClose(hFind);
	
	return result;
}
