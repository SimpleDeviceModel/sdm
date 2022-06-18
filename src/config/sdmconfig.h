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
 * This header file declares a set of functions that provide access
 * to data configured by the build system.
 */


#ifndef SDMDIRS_H_INCLUDED
#define SDMDIRS_H_INCLUDED

#include "dirutil.h"

#include <ctime>

namespace Config {
	const char *version();
	const char *shortVersion();
	const char *commitHash();
	std::time_t commitTimestamp();
	const char *compiler();
	const char *os();
	const char *architecture();
	
	Path installPrefix();
	Path binDir();
	Path pluginsDir();
	Path luaModulesDir();
	Path luaCModulesDir();
	Path qtDir();
	Path docDir();
	Path translationsDir();
	Path scriptsDir();
	Path dataDir();

	std::string luaModulePath();
	std::string luaCModulePath(const std::string &versionSuffix="");
}

#endif
