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
 * Implements functions from the StringUtils namespace.
 */

#include "stringutils.h"

#include <cctype>
#include <stdexcept>

namespace StringUtils {
	std::vector<std::string> splitString(const std::string &str,char sep) {
		std::vector<std::string> res;
		
		if(str.empty()) return res;
	
		for(std::size_t begin=0;;) {
			auto pos=str.find_first_of(sep,begin);
			if(pos==std::string::npos) {
				res.push_back(str.substr(begin));
				break;
			}
			res.push_back(str.substr(begin,pos-begin));
			begin=pos+1;
		}
		
		return res;
	}
	
	std::string cleanupString(const std::string &str) {
		if(str.empty()) return str;
		auto begin=str.find_first_not_of(" \t\r\n");
		auto end=str.find_last_not_of(" \t\r\n");
		if(begin==std::string::npos||end==std::string::npos) return std::string();
		return str.substr(begin,end-begin+1);
	}
	
	int compareVersions(const std::string &current,const std::string &required) try {
		auto const currentComponents=splitString(current,'.');
		auto const requiredComponents=splitString(required,'.');
		
		for(std::size_t i=0;i<requiredComponents.size();i++) {
			if(i>=currentComponents.size()) return -1;
			const int c=std::stoi(currentComponents[i]);
			const int r=std::stoi(requiredComponents[i]);
			if(c<r) return -1;
			else if(c>r) return 1;
		}
		
		return 0;
	}
	catch(std::exception &) {
		throw std::runtime_error("Malformed version string");
	}
	
	bool endsWith(const std::string &str,const std::string &suffix,bool caseInsensitive) {
		auto s1=str;
		auto s2=suffix;
		if(caseInsensitive) {
			for(auto &ch: s1) ch=std::tolower(ch);
			for(auto &ch: s2) ch=std::tolower(ch);
		}
		if(s2.size()>s1.size()) return false;
		auto pos=s1.size()-s2.size();
		return (s1.compare(pos,s2.size(),s2)==0);
	}
}
