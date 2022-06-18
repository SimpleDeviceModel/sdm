/*
 * Copyright (c) 2015-2022 Simple Device Model contributors
 * 
 * This file is part of the Simple Device Model (SDM) framework SDK.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * This module provides an implementation for the SDMPropertyManager
 * class members.
 */

#include "sdmproperty.h"

#include <stdexcept>

/*
 * Public members
 */

SDMPropertyManager::SDMPropertyManager(): _dirty(false) {}

SDMPropertyManager::~SDMPropertyManager() {}

void SDMPropertyManager::clear() {
	_props.clear();
	_listall.clear();
	_dirty=true;
}

void SDMPropertyManager::addProperty(const std::string &name,const std::string &value) {
	std::map<std::string,std::pair<std::string,Type> >::iterator it=_props.find(name);
	if(it==_props.end()) {
		_props[name]=std::make_pair(value,Normal);
		_listall.push_back(name);
	}
	else it->second=std::make_pair(value,Normal);
	_dirty=true;
}

void SDMPropertyManager::addConstProperty(const std::string &name,const std::string &value) {
	std::map<std::string,std::pair<std::string,Type> >::iterator it=_props.find(name);
	if(it==_props.end()) {
		_props[name]=std::make_pair(value,ReadOnly);
		_listall.push_back(name);
	}
	else it->second=std::make_pair(value,ReadOnly);
	_dirty=true;
}

void SDMPropertyManager::addListItem(const std::string &list,const std::string &item) {
	std::map<std::string,std::pair<std::string,Type> >::iterator it=_props.find(list);
	if(it==_props.end()) {
		_props[list]=std::make_pair(formatListItem(item),List);
		_listall.push_back(list);
	}
	else if(it->second.second!=List) it->second=std::make_pair(formatListItem(item),List);
	else it->second.first+=(","+formatListItem(item));
	_dirty=true;
}

std::string SDMPropertyManager::getProperty(const std::string &name) const {
// Handle predefined lists
	if(name=="*") {
		if(_dirty) rebuildCache();
		return _cacheAll;
	}
	if(name=="*ro") {
		if(_dirty) rebuildCache();
		return _cacheReadOnly;
	}
	if(name=="*wr") {
		if(_dirty) rebuildCache();
		return _cacheWritable;
	}
	
// Handle normal properties
	std::map<std::string,std::pair<std::string,Type> >::const_iterator it=_props.find(name);
	if(it!=_props.end()) return it->second.first;
	throw std::runtime_error("Property \""+name+"\" not found");
}

std::string SDMPropertyManager::getProperty(const std::string &name,const std::string &defaultValue) const {
	try {
		return getProperty(name);
	}
	catch(std::exception &) {
		return defaultValue;
	}
}

void SDMPropertyManager::setProperty(const std::string &name,const std::string &value) {
	std::map<std::string,std::pair<std::string,Type> >::iterator it=_props.find(name);
	if(it==_props.end()) throw std::runtime_error("Property \""+name+"\" is not defined");
	if(it->second.second!=Normal) throw std::runtime_error("Property \""+name+"\" is not writable");
	it->second.first=value;
}

/*
 * Private members
 */

void SDMPropertyManager::rebuildCache() const {
	std::map<std::string,std::pair<std::string,Type> >::const_iterator it;
	
	_cacheAll.clear();
	_cacheReadOnly.clear();
	_cacheWritable.clear();
	for(std::size_t i=0;i<_listall.size();i++) {
		it=_props.find(_listall[i]);
		if(it==_props.end()) continue;
		
		const std::string &listItem=formatListItem(_listall[i]);
		
		if(!_cacheAll.empty()) _cacheAll.push_back(',');
		_cacheAll+=listItem;
		
		if(it->second.second!=Normal) {
			if(!_cacheReadOnly.empty()) _cacheReadOnly.push_back(',');
			_cacheReadOnly+=listItem;
		}
		else {
			if(!_cacheWritable.empty()) _cacheWritable.push_back(',');
			_cacheWritable+=listItem;
		}
	}
	
	_dirty=false;
}

std::string SDMPropertyManager::formatListItem(const std::string &str) {
	std::string res("\"");
	for(std::size_t i=0;i<str.size();i++) {
		if(str[i]=='\"') res+="\"\"";
		else res.push_back(str[i]);
	}
	res.push_back('\"');
	return res;
}
