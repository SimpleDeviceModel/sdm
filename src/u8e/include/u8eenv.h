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
 * This header file declares a function which can be used to obtain
 * command line arguments as well as get/set environment variables
 * in arbitrary encoding (default is UTF-8).
 */

#ifndef U8EENV_H_INCLUDED
#define U8EENV_H_INCLUDED

#include "u8ecodec.h"

#include <vector>
#include <string>

namespace u8e {
	std::vector<std::string> cmdArgs(int argc,char *argv[],Encoding enc=UTF8);

	std::string envVar(const std::string &name,Encoding enc=UTF8);
	void setEnvVar(const std::string &name,const std::string &value,Encoding enc=UTF8);
	void delEnvVar(const std::string &name,Encoding enc=UTF8);
}

#endif
