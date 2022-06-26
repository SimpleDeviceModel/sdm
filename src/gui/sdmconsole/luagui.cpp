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
 * This module implements members of the LuaGUIObject class.
 */

#include "luagui.h"
#include "luaserver.h"
#include "marshal.h"

#include <cassert>

using namespace std::placeholders;

/*
 * LuaGUIObject members
 */

const LuaGUIObject::InvokeType LuaGUIObject::Normal=0;
const LuaGUIObject::InvokeType LuaGUIObject::Async=1;
const LuaGUIObject::InvokeType LuaGUIObject::TableAsArray=2;

LuaGUIObject::LuaGUIObject() {
	proxyInvoker=std::bind(&LuaGUIObject::proxy,this,_1);
	_thread=QThread::currentThread();
}

std::function<int(LuaServer&)> LuaGUIObject::enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
	if(invokers.empty()) {
// First call. Initialize myself
		for(int j=0;;j++) {
			invokers.emplace_back();
			invokers.back().invoker=enumerateGUIMethods(j,invokers.back().name,invokers.back().type);
			if(!invokers.back().invoker) {
				invokers.pop_back();
				break;
			}
		}
// Override LuaCallbackObject's garbage collection and "to close" handlers
// because we need to be destroyed from the QObject's thread
		if(owner()) { // we are managed object
			invokers.emplace_back();
			invokers.back().invoker=std::bind(&LuaGUIObject::unsafeDestroy,this,_1,_2);
			invokers.back().name="__gc";
			invokers.back().type=Async; // object is already unaccessible, no need to wait
			invokers.emplace_back();
			invokers.back().invoker=std::bind(&LuaGUIObject::unsafeDestroy,this,_1,_2);
			invokers.back().name="__close";
			invokers.back().type=Async; // object is already unaccessible, no need to wait
		}
	}

	if(i>=0&&i<static_cast<int>(invokers.size())) {
		strName=invokers[i].name;
		upvalues.emplace_back(static_cast<lua_Integer>(i));
		return proxyInvoker;
	}
	return std::function<int(LuaServer&)>();
}

/*
 * LuaGUIObject::proxy() is invoked by Lua. It uses generalized marshaling (see
 * marshal.h) to dispatch the call;
 */

int LuaGUIObject::proxy(LuaServer &lua) {
	assert(lua.nupv()>=1);
	int index=static_cast<int>(lua.upvalues(0).toInteger());
	assert(index>=0&&index<static_cast<int>(invokers.size()));
	
	bool tableAsArray=static_cast<bool>(invokers[index].type&TableAsArray);
	std::vector<LuaValue> ia;
	for(int i=0;i<lua.argc();i++) ia.push_back(lua.argv(i,tableAsArray));
	
	auto functor=std::bind(invokers[index].invoker,std::ref(lua),std::move(ia));
	
	if(invokers[index].type&Async) {
		marshalAsync(std::move(functor));
		return 0;
	}
	else return marshal(std::move(functor));
}

int LuaGUIObject::unsafeDestroy(LuaServer &lua,const std::vector<LuaValue> &) {
	if(owner()) {
		enableUnsafeDestruction(true); // enable unregistration while callback is in progress
		delete this;
	}
	return 0;
}
