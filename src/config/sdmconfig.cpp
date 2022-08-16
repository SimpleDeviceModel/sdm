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
 * This module implements the functions of the Config namespace.
 */

#include "sdmconfig.h"
#include "u8ecodec.h"

#include "sdmdirscfg.h"
#include "sdmvercfg.h"

#include <iostream>
#include <sstream>
#include <cassert>

#ifdef _WIN32
	#define SHARED_EXT "dll"
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <shlobj.h>
#else
	#define SHARED_EXT "so"
#endif

const char *Config::version() {
	return SDM_VERSION;
}

const char *Config::shortVersion() {
	return SDM_SHORTVERSION;
}

const char *Config::commitHash() {
	return SDM_COMMIT_HASH;
}

std::time_t Config::commitTimestamp() {
	return SDM_COMMIT_TIMESTAMP;
}

const char *Config::compiler() {
	return SDM_COMPILER;
}

const char *Config::os() {
	return SDM_OS;
}

const char *Config::architecture() {
	return SDM_ARCH;
}

Path Config::installPrefix() {
	Path exe=Path::exePath();
	if(!exe.empty()) {
		static int bindircount=Path(BIN_INSTALL_DIR).count();
		assert(bindircount>=0);
		return exe.up(bindircount+1);
	}
// Use fallback directory hardcoded by the build system
	return Path(INSTALL_PREFIX_FALLBACK);
}

Path Config::binDir() {
	return installPrefix()+BIN_INSTALL_DIR;
}

Path Config::pluginsDir() {
	return installPrefix()+PLUGINS_INSTALL_DIR;
}

Path Config::luaModulesDir() {
	return installPrefix()+LUA_MODULES_INSTALL_DIR;
}

Path Config::luaCModulesDir() {
	return installPrefix()+LUA_CMODULES_INSTALL_DIR;
}

Path Config::qtDir() {
	return installPrefix()+QT_INSTALL_DIR;
}

Path Config::docDir() {
	return installPrefix()+DOC_INSTALL_DIR;
}

Path Config::translationsDir() {
	return installPrefix()+I18N_INSTALL_DIR;
}

Path Config::scriptsDir() {
	return installPrefix()+SCRIPTS_INSTALL_DIR;
}

Path Config::dataDir() {
	return installPrefix()+DATA_INSTALL_DIR;
}

#ifdef _WIN32

Path Config::appConfigDir() {
	std::vector<wchar_t> buf(MAX_PATH+1);
	auto r=SHGetFolderPathW(NULL,CSIDL_APPDATA,NULL,SHGFP_TYPE_CURRENT,buf.data());
	if(r!=S_OK) { // something is wrong, return the default location
		return Path::home()+"AppData\\Roaming\\Simple Device Model";
	}
	u8e::WCodec codec(u8e::UTF8);
	return Path(codec.transcode(buf.data()))+"Simple Device Model";
}

#else

Path Path::appConfigDir() {
	return Path::home()+".config/Simple Device Model";
}

#endif

/*
 * SDM represents file names in UTF-8, but Lua won't handle it.
 * luaModulePath() and luaCModulePath() return strings in
 * a locale-dependent multibyte encoding ("ANSI" on Windows).
 */

std::string Config::luaModulePath() {
	u8e::Codec codec(u8e::UTF8,u8e::LocalMB);
	Path modulesDir=Path(codec.transcode(luaModulesDir().str()));
	
	std::vector<Path> paths={modulesDir};
	std::vector<std::string> fileNames={"?.lua",std::string("?")+Path::sep()+"init.lua"};

	std::ostringstream oss;
	
	for(auto const &path: paths) {
		for(auto const &name: fileNames) {
			oss<<(path+name).str()<<';';
		}
	}
	
	return oss.str();
}

std::string Config::luaCModulePath(const std::string &versionSuffix) {
	u8e::Codec codec(u8e::UTF8,u8e::LocalMB);
	Path modulesDir=Path(codec.transcode(luaCModulesDir().str()));
	
	std::vector<Path> paths={modulesDir};
	std::vector<std::string> fileNames={"?." SHARED_EXT,"lib?." SHARED_EXT};
	if(!versionSuffix.empty()) {
		fileNames.push_back("?"+versionSuffix+("." SHARED_EXT));
		fileNames.push_back("lib?"+versionSuffix+("." SHARED_EXT));
	}
	fileNames.push_back("loadall." SHARED_EXT);

	std::ostringstream oss;
	
	for(auto const &path: paths) {
		for(auto const &name: fileNames) {
			oss<<(path+name).str()<<';';
		}
	}
	
	return oss.str();
}
