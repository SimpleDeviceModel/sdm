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
 * This header file defines a cross-platform class for loading
 * external modules. File names are assumed to be in UTF-8.
 */

#ifndef LOADABLEMODULE_H_INCLUDED
#define LOADABLEMODULE_H_INCLUDED

#include <string>
#include <memory>

class LoadableModule final {
public:
	typedef void(*GenericFuncPtr)();
private:
	class LoadableModuleImpl;
	std::shared_ptr<LoadableModuleImpl> _impl;
public:
	LoadableModule();
	LoadableModule(const std::string &moduleName);
	
	void load(const std::string &moduleName);
	void unload();
	
	operator bool() const;
	std::string path() const;
	
	GenericFuncPtr getAddr(const std::string &symbolName);
};

#endif
