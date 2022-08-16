/*
 * Copyright (c) 2015-2022 Simple Device Model contributors
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
 * Defines an implementation of the Path class which provides
 * platform-independent path management facilities.
 */

#include "dirutil.h"
#include "u8ecodec.h"
#include "u8eenv.h"

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <userenv.h>
#else
	#include <unistd.h>
	#include <limits.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <pwd.h>
#endif

#include <sstream>
#include <cstdlib>
#include <cctype>

Path::Path(const std::string &str) {
#ifdef _WIN32
	const char *separators="\\/";
#else
	const char *separators="/";
#endif

	if(str.empty()) return;
// Dissect path
	std::size_t i1=0,i2;
	for(;;) {
		i2=str.find_first_of(separators,i1);
		if(i2==std::string::npos) {
			path.push_back(str.substr(i1));
			break;
		}
		else if(i2==str.size()-1) {
			path.push_back(str.substr(i1,i2-i1));
			break;
		}
		path.push_back(str.substr(i1,i2-i1));
		i1=i2+1;
	}

// Normalize
	normalize();
}

Path Path::operator+(const Path &right) const {
	if(path.empty()||right.isAbsolute()) return right;
	if(right.empty()) return *this;
	Path res(*this);
	res.path.insert(res.path.end(),right.path.begin(),right.path.end());
	res.normalize();
	return res;
}

bool Path::operator==(const Path &right) const {
	if(path.size()!=right.path.size()) return false;
	for(std::size_t i=0;i<path.size();i++) {
// Windows uses case-insensitive paths
#ifdef _WIN32
		if(tolower(path[i])!=tolower(right.path[i])) return false;
#else
		if(path[i]!=right.path[i]) return false;
#endif
	}
	return true;
}

bool Path::isAbsolute() const {
	if(path.empty()) return false;
	if(path[0].empty()) return true;
#ifdef _WIN32
	if(path[0].size()>1) if(path[0][1]==':') return true;
#endif
	return false;
}

std::string Path::str() const {
	if(path.empty()) return ".";
	std::ostringstream ostr;
	for(std::size_t i=0;i<path.size();i++) {
		ostr<<path[i];
		if(i+1!=path.size()||path[i].empty()) ostr<<sep();
	}
	return ostr.str();
}

char Path::sep() {
#ifdef _WIN32
	return '\\';
#else
	return '/';
#endif
}

Path Path::toAbsolute() const {
	if(isAbsolute()) return *this;
	return Path::current()+(*this);
}

Path Path::up(int n) const {
	Path res(*this);
	for(int i=0;i<n;i++) res+="..";
	return res;
}

int Path::count() const {
	int res=0;
	for(auto it=path.cbegin();it!=path.cend();it++) {
		if((*it)!="..") res++;
		else res--;
	}
	return res;
}

#ifdef _WIN32

void Path::mkdir() const {
	auto const &abs=toAbsolute();
	if(abs.count()>2) abs.up().mkdir();
	u8e::WCodec codec(u8e::UTF8);
	CreateDirectoryW(codec.transcode(abs.str()).c_str(),nullptr);
}

Path Path::exePath() {
	std::vector<wchar_t> moduleName(1024);
	
	for(;;) {
		DWORD dw=GetModuleFileNameW(NULL,&moduleName[0],static_cast<DWORD>(moduleName.size()));
		if(!dw) return Path();
		else if(dw==moduleName.size()) { // insufficient buffer, enlarge
			if(moduleName.size()>=32768) return Path();
			moduleName.resize(moduleName.size()*2);
		}
		else break;
	}
	
	u8e::WCodec codec(u8e::UTF8);
	return Path(codec.transcode(&moduleName[0]));
}

Path Path::current() {
	std::size_t bufsize=GetCurrentDirectoryW(0,NULL);
	std::vector<wchar_t> buf(bufsize);
	DWORD dw=GetCurrentDirectoryW(static_cast<DWORD>(bufsize),&buf[0]);
	if(dw==0||dw==bufsize) return Path();
	u8e::WCodec codec(u8e::UTF8);
	return Path(codec.transcode(&buf[0]));
}

Path Path::home() {
	HANDLE token;
	BOOL b=OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token);
	if(!b) return Path();
	DWORD bufsize=0;
	GetUserProfileDirectoryW(token,NULL,&bufsize);
	if(!bufsize) {
		CloseHandle(token);
		return Path();
	}
	std::vector<wchar_t> buf(bufsize);
	b=GetUserProfileDirectoryW(token,&buf[0],&bufsize);
	CloseHandle(token);
	if(!b) return Path();
	u8e::WCodec codec(u8e::UTF8);
	return Path(codec.transcode(&buf[0]));
}

#else

void Path::mkdir() const {
	auto const &abs=toAbsolute();
	if(abs.count()>2) abs.up().mkdir();
	u8e::Codec codec(u8e::UTF8,u8e::LocalMB);
	::mkdir(codec.transcode(abs.str()).c_str(),0777);
}

Path Path::exePath() {
	u8e::Codec codec(u8e::LocalMB,u8e::UTF8);
// Note: this code is Linux-specific
	Path res;
	char *sz=realpath("/proc/self/exe",NULL);
	if(sz) {
		try {
			res=codec.transcode(sz);
		}
		catch(...) {
			free(sz);
			throw;
		}
		free(sz);
		return res;
	}
	return Path();
}

Path Path::current() {
	u8e::Codec codec(u8e::LocalMB,u8e::UTF8);
	char buf[PATH_MAX+1];
	char *sz=getcwd(buf,PATH_MAX+1);
	if(sz) return Path(codec.transcode(buf));
	return Path();
}

Path Path::home() {
// Try to get the value of the HOME environment variable
// (this is preferred since the user can change it)
	auto homeVar=u8e::envVar("HOME");
	if(!homeVar.empty()) return Path(homeVar);
// Otherwise, get the initial home directory using getpwuid()
	auto pwd=getpwuid(getuid());
	if(!pwd) return Path();
	auto initialHomeDir=pwd->pw_dir;
	if(!initialHomeDir) return Path();
	u8e::Codec codec(u8e::LocalMB,u8e::UTF8);
	return Path(codec.transcode(initialHomeDir));
}

#endif

void Path::normalize() {
	for(auto it=path.begin();it!=path.end();) {
		if(it->empty()) {
			if(it==path.begin()) it++; // path is absolute - OK
			else if(it-1==path.begin()&&(it-1)->empty()) it++; // UNC path on Windows - OK
			else it=path.erase(it); // delete extra separators
		}
		else if(*it==".") it=path.erase(it);
		else if(*it==".."&&it!=path.begin()) {
			if(!(it-1)->empty()&&*(it-1)!="..") it=path.erase(it-1,it+1);
			else if((it-1)->empty()) it=path.erase(it); // don't go upper than root
			else it++;
		}
		else it++;
	}
}

std::string Path::tolower(const std::string &str) {
	std::string res;
	for(std::size_t i=0;i<str.size();i++) res.push_back(std::tolower(str[i]));
	return res;
}
