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
 * This module provides an implementation of the LoadableModule class.
 */

#include "loadablemodule.h"
#include "stringutils.h"
#include "u8ecodec.h"

#include <cstdlib>
#include <sstream>
#include <vector>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <windows.h>
	#include <psapi.h>
#else // not Windows
	#include <dlfcn.h>
#endif

/*
 * ModuleImpl definition
 */

class LoadableModule::LoadableModuleImpl {
#ifdef _WIN32
	HMODULE _module=NULL;
	std::vector<HMODULE> _pinnedModules;
#else
	void *_module=RTLD_DEFAULT;
#endif
	std::string _path;
public:
	LoadableModuleImpl() {}
	LoadableModuleImpl(const std::string &moduleName);
	LoadableModuleImpl(const LoadableModuleImpl &)=delete;
	LoadableModuleImpl(LoadableModuleImpl &&)=delete;
	~LoadableModuleImpl();
	
	LoadableModuleImpl &operator=(const LoadableModuleImpl &)=delete;
	LoadableModuleImpl &operator=(LoadableModuleImpl &&)=delete;
	
	GenericFuncPtr getAddr(const std::string &symbolName);
	
	operator bool() const;
	std::string path() const {return _path;}
private:
	static bool appendExtension(std::string &name);
#ifdef _WIN32
	GenericFuncPtr rawGetAddr(HMODULE module,const std::string &symbolName);
	GenericFuncPtr findModule(const std::string &symbolName);
#endif
};

/*
 * ModuleImpl members
 */

#ifdef _WIN32

static const std::string SharedExt="dll";

static bool errorModeInit=false;

LoadableModule::LoadableModuleImpl::LoadableModuleImpl(const std::string &moduleName) {
	if(moduleName.empty()) return;
	
	_path=moduleName;
	
// Set error mode to avoid unneeded Windows error messages
	if(!errorModeInit) {
		UINT flagsToSet=SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX;
		auto oldMode=SetErrorMode(flagsToSet);
		SetErrorMode(oldMode|flagsToSet);
		errorModeInit=true;
	}

/*
 * Note: if the file name is specified without a path and extension,
 * LoadLibrary will append the "dll" extension automatically. This
 * behavior can be prevented by appending "." to the file name
 * (see https://msdn.microsoft.com/en-us/library/windows/desktop/ms684175(v=vs.85).aspx)
 *
 * However, if the file name already ends with ".dll", appending
 * an extra "." is undesirable. While the DLL will still load, doing
 * so for a system DLL (such as kernel32.dll) on Windows XP causes
 * multiple copies of that DLL to be loaded (at different addresses),
 * which leads to unpredictable behavior and crashes when these copies
 * are eventually unloaded. This issue has not been observed on
 * Windows 8.1.
 *
 * Therefore, we never append extra "." to the module file name.
 * If the user wants to load a module without an extension, they
 * must indicate that explicitly by including the trailing dot.
 */

// Load the module
	u8e::WCodec codec(u8e::UTF8);
	_module=LoadLibraryW(codec.transcode(_path).c_str());
	if(!_module) {
		if(appendExtension(_path)) {
			codec.reset();
			_module=LoadLibraryW(codec.transcode(_path).c_str());
		}
	}
	if(!_module) {
		_path.clear();
		throw std::runtime_error("Cannot load module \""+moduleName+
			"\": Windows error code "+std::to_string(GetLastError()));
	}
	
// Try to get real module path
	std::vector<wchar_t> realPath(1024);
	
	for(;;) {
		DWORD dw=GetModuleFileNameW(_module,realPath.data(),static_cast<DWORD>(realPath.size()));
		if(!dw) break;
		else if(dw==realPath.size()) { // insufficient buffer, enlarge
			if(realPath.size()>=32768) break;
			realPath.resize(realPath.size()*2);
		}
		else {
			_path=codec.transcode(realPath.data());
			break;
		}
	}
}

LoadableModule::LoadableModuleImpl::~LoadableModuleImpl() {
	if(_module) FreeLibrary(_module);
	for(auto module: _pinnedModules) FreeLibrary(module);
}

LoadableModule::GenericFuncPtr LoadableModule::LoadableModuleImpl::getAddr(const std::string &symbolName) {
	if(symbolName.empty()) throw std::runtime_error("Symbol name is empty");
	if(_module) return rawGetAddr(_module,symbolName);
	else return findModule(symbolName);
}

LoadableModule::LoadableModuleImpl::operator bool() const {
	return (_module!=NULL);
}

LoadableModule::GenericFuncPtr LoadableModule::LoadableModuleImpl::rawGetAddr(HMODULE module,const std::string &symbolName) {
	if(symbolName.empty()) throw std::runtime_error("Symbol name is empty");
	
	FARPROC func=nullptr;
	std::ostringstream errmsg;
	
// If symbolName is a number in the range 1-65535, try ordinal first
	char *endptr;
	auto ordinal=std::strtol(symbolName.c_str(),&endptr,0);
	if(*endptr=='\0'&&ordinal>=1&&ordinal<=65535) {
		func=GetProcAddress(module,reinterpret_cast<const char*>(static_cast<WORD>(ordinal)));
		if(!func) errmsg<<"Cannot locate ordinal "<<std::to_string(ordinal)<<std::endl;
	}
	
	if(!func) {
		func=GetProcAddress(module,symbolName.c_str());
		if(!func) {
			errmsg<<"Cannot locate symbol \""<<symbolName<<"\"";
			throw std::runtime_error(errmsg.str());
		}
	}
	
	return reinterpret_cast<GenericFuncPtr>(func);
}

LoadableModule::GenericFuncPtr LoadableModule::LoadableModuleImpl::findModule(const std::string &symbolName) {
// Try modules that are already pinned
	for(auto module: _pinnedModules) {
		try {return rawGetAddr(module,symbolName);}
		catch(std::exception &) {}
	}
	
// Enumerate modules loaded by the current process
	std::vector<HMODULE> modules(1024);
	
	for(;;) {
		DWORD bytes;
		BOOL r=EnumProcessModules(GetCurrentProcess(),
			modules.data(),
			static_cast<DWORD>(modules.size()*sizeof(HMODULE)),
			&bytes);
		
		if(!r) throw std::runtime_error("Cannot enumerate loaded modules");
		auto modcnt=bytes/sizeof(HMODULE);
		if(modcnt<=modules.size()) {
			modules.resize(modcnt);
			break;
		}
		modules.resize(modcnt);
	}
	
// Try to find the requested symbol in loaded modules
	for(auto module: modules) {
		HMODULE h;
// Note: the following function increments library reference counter
		BOOL r=GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCTSTR>(module),&h);
		if(!r) continue;
		try {
			auto func=rawGetAddr(h,symbolName);
			_pinnedModules.push_back(h);
			return func;
		}
		catch(std::exception &) {
			FreeLibrary(h);
		}
	}
	throw std::runtime_error("Cannot locate symbol \""+symbolName+"\"");
}

#else // not Windows

static const std::string SharedExt="so";

LoadableModule::LoadableModuleImpl::LoadableModuleImpl(const std::string &moduleName) {
	if(moduleName.empty()) return; // use RTLD_DEFAULT
	_path=moduleName;
	u8e::Codec codec(u8e::UTF8,u8e::LocalMB);
	std::string errmsg;
	_module=dlopen(codec.transcode(_path).c_str(),RTLD_LAZY|RTLD_GLOBAL);
	if(!_module) {
		errmsg=dlerror();
		if(appendExtension(_path)) {
			codec.reset();
			_module=dlopen(codec.transcode(_path).c_str(),RTLD_LAZY|RTLD_GLOBAL);
		}
	}
	if(!_module) {
		_path.clear();
		_module=RTLD_DEFAULT;
		throw std::runtime_error("Cannot load module \""+moduleName+"\": "+errmsg);
	}
}

LoadableModule::LoadableModuleImpl::~LoadableModuleImpl() {
	if(_module!=RTLD_DEFAULT) dlclose(_module);
}

LoadableModule::GenericFuncPtr LoadableModule::LoadableModuleImpl::getAddr(const std::string &symbolName) {
	auto func=dlsym(_module,symbolName.c_str());
	if(!func) throw std::runtime_error("Cannot locate symbol \""+symbolName+"\": "+dlerror());
	static_assert(sizeof(GenericFuncPtr)==sizeof(void*),"Function and object pointer sizes differ");
	GenericFuncPtr res;
	std::memcpy(&res,&func,sizeof(GenericFuncPtr));
	return res;
}

LoadableModule::LoadableModuleImpl::operator bool() const {
	return (_module!=RTLD_DEFAULT);
}

#endif

bool LoadableModule::LoadableModuleImpl::appendExtension(std::string &name) {
	if(name.empty()) return false;
	if(StringUtils::endsWith(name,"."+SharedExt,true)) return false;
	if(name.back()!='.') name.push_back('.');
	name+=SharedExt;
	return true;
}

/*
 * LoadableModule members
 */

LoadableModule::LoadableModule(): _impl(std::make_shared<LoadableModuleImpl>()) {}

LoadableModule::LoadableModule(const std::string &moduleName):
	_impl(std::make_shared<LoadableModuleImpl>(moduleName)) {}

void LoadableModule::load(const std::string &moduleName) {
	_impl.reset(new LoadableModuleImpl(moduleName));
}

void LoadableModule::unload() {
	_impl.reset(new LoadableModuleImpl);
}

LoadableModule::operator bool() const {
	return _impl->operator bool();
}

std::string LoadableModule::path() const {
	return _impl->path();
}

LoadableModule::GenericFuncPtr LoadableModule::getAddr(const std::string &symbolName) {
	return _impl->getAddr(symbolName);
}
