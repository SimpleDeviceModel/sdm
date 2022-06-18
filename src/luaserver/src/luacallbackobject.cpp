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
 * LuaServer is a small wrapper library integrating Lua interpreter
 * into a C++ program.
 *
 * This module provides an implementation of the LuaCallbackObject
 * class members.
 */

#include "luacallbackobject.h"
#include "luaserver.h"

#include <algorithm>

/*
 * Public members
 */

LuaCallbackObject::LuaCallbackObject(): _cb(std::make_shared<ControlBlock>()) {}

// LuaCallbackObject can be copied and moved, but each instance must be registered explicilty

LuaCallbackObject::LuaCallbackObject(const LuaCallbackObject &): LuaCallbackObject() {}

LuaCallbackObject::LuaCallbackObject(LuaCallbackObject &&): LuaCallbackObject() {}

LuaCallbackObject::~LuaCallbackObject() {
	if(_cb->owner) _cb->owner->detachManagedObject(this);
}

LuaCallbackObject &LuaCallbackObject::operator=(const LuaCallbackObject &) {
	return *this;
}

LuaCallbackObject &LuaCallbackObject::operator=(LuaCallbackObject &&) {
	return *this;
}

/*
 * Protected members
 */

void LuaCallbackObject::detach() {
	if(_cb->owner) {
		_cb->owner->detachManagedObject(this);
		_cb->owner=nullptr;
	}
}

void LuaCallbackObject::enableUnsafeDestruction(bool b) {
	if(!b) _cb->enableUnsafeDestruction=false;
	else {
		_cb->unsafeDestructionThread=std::this_thread::get_id();
		_cb->enableUnsafeDestruction=true;
	}
}

/*
 * Private members
 */

int LuaCallbackObject::gcDispose(LuaServer &) {
	delete this;
	return 0;
}
