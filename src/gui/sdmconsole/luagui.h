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
 * This header file defines a generic Lua callback class to be
 * used with Qt GUI objects. It transparently handles inter-thread
 * communication, ensuring that GUI object methods are called
 * from the main thread.
 * 
 * To use, inherit from LuaGUIObject and implement enumerateGUIMethods()
 * pure virtual function. Each method can be declared as synchronous
 * or asynchronous. For asynchronous methods, Lua will not wait for
 * completion when invoked from another thread. Accessing Lua server
 * from asynchronous methods results in undefined behavior.
 */

#ifndef LUAGUI_H_INCLUDED
#define LUAGUI_H_INCLUDED

#include "luacallbackobject.h"
#include "marshal.h"

#include "safeflags.h"

#include <vector>
#include <string>

/*
 * Main class definition
 */

class LuaGUIObject : public LuaCallbackObject,public Marshal {
public:	
	typedef std::function<int(LuaServer &,const std::vector<LuaValue> &)> Invoker;
	
	typedef SafeFlags<LuaGUIObject> InvokeType;

	static const InvokeType Normal;
	static const InvokeType Async;
	static const InvokeType TableAsArray;

private:
	struct InvokeData;
	
	std::vector<InvokeData> invokers;
	std::function<int(LuaServer&)> proxyInvoker;
	QThread *_thread;
	
	int proxy(LuaServer &lua);
	int unsafeDestroy(LuaServer &lua,const std::vector<LuaValue> &);
public:
	LuaGUIObject();
	virtual ~LuaGUIObject() {}

protected:
	virtual std::function<int(LuaServer&)> enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) override;
	virtual Invoker enumerateGUIMethods(int i,std::string &strName,InvokeType &type)=0;
};

/*
 * Helper structures
 */

struct LuaGUIObject::InvokeData {
	Invoker invoker;
	std::string name;
	InvokeType type;
};

#endif
