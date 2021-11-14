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
 * SDMPlug is a C++ library intended to be used by host applications
 * to access SDM plugins.
 */

#include "sdmplug.h"
#include "csvparser.h"

#include <stdexcept>
#include <sstream>

/*
 * sdmplugin_error members
 */

sdmplugin_error::sdmplugin_error(const std::string &strFuncName) {
	std::ostringstream ss;
	ss<<"SDM plugin function "<<strFuncName<<" returned an error";
	message=ss.str();
}

sdmplugin_error::sdmplugin_error(const std::string &strFuncName, int iErrorCode) {
	std::ostringstream ss;
	ss<<"SDM plugin function "<<strFuncName<<" returned an error code "<<iErrorCode;
	message=ss.str();
	code=iErrorCode;
}

/*
 * SDMBase members
 */

std::string SDMBase::getProperty(const std::string &name) {
	char smallbuf[256];
	
// Try to use fixed-size buffer
	int size=getPropertyAPI(name.c_str(),smallbuf,256);
	if(size==0) return std::string(smallbuf);
	if(size<0) throw std::runtime_error("Can't get property \""+name+"\"");

// Otherwise, allocate buffer of required size
	std::vector<char> buf(size);

// Get property value
	int r=getPropertyAPI(name.c_str(),buf.data(),size);
	if(r!=0) throw std::runtime_error("Can't get property \""+name+"\"");
	return std::string(buf.data());
}

std::string SDMBase::getProperty(const std::string &name,const std::string &defaultValue) try {
	return getProperty(name);
}
catch(std::exception &) {
	return defaultValue;
}

std::vector<std::string> SDMBase::listProperties(const std::string &name) {
	std::string val(getProperty(name,""));
	std::istringstream iss(val);
	return CSVParser::getRecord(iss);
}

void SDMBase::setProperty(const std::string &name,const std::string &value) {
	int r=setPropertyAPI(name.c_str(),value.c_str());
	if(r) throw std::runtime_error("Can't set property \""+name+"\"");
}
