/*
 * Copyright (c) 2015-2021 by Microproject LLC
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
 * This header file defines a helper class to work with SDM
 * properties. It is intended to be used in SDM plugins.
 * 
 * Note: All the exceptions thrown by SDMPropertyManager members
 * are derived from std::exception.
 */

#ifndef SDMPROPERTY_H_INCLUDED
#define SDMPROPERTY_H_INCLUDED

#include <map>
#include <string>
#include <vector>
#include <utility>

class SDMPropertyManager {
	enum Type {Normal,ReadOnly,List};
	
	std::map<std::string,std::pair<std::string,Type> > _props;
	std::vector<std::string> _listall;
	
	mutable std::string _cacheAll;
	mutable std::string _cacheReadOnly;
	mutable std::string _cacheWritable;
	mutable bool _dirty;

public:
	SDMPropertyManager();
	virtual ~SDMPropertyManager();

// Reset the object, deleting all properties and lists.
	void clear();

// Define a new editable property or redefine the existing one.
	void addProperty(const std::string &name,const std::string &value);
// Define a new read-only property or redefine the existing one.
	void addConstProperty(const std::string &name,const std::string &value);
// Add a new item to the list. If the list doesn't exist, create it.
	void addListItem(const std::string &list,const std::string &item);

// Get the value of the existing property. Throw an exception if the property doesn't exist.
	virtual std::string getProperty(const std::string &name) const;
// Get the value of the existing property. Return the default value if the property doesn't exist.
	virtual std::string getProperty(const std::string &name,const std::string &defaultValue) const;
// Set the value of the existing property. Throw an exception if the property doesn't exist or is not writable.
	virtual void setProperty(const std::string &name,const std::string &value);
	
private:
	void rebuildCache() const;
	static std::string formatListItem(const std::string &str);
};

#endif
