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
 * This header file defines the LuaCallbackObject class.
 */

#ifndef LUACALLBACKOBJECT_H_INCLUDED
#define LUACALLBACKOBJECT_H_INCLUDED

#include "luavalue.h"

#include <memory>
#include <mutex>
#include <functional>
#include <atomic>
#include <thread>
#include <vector>

class LuaServer;

class LuaCallbackObject {
	friend class LuaServer;
public:
	typedef std::recursive_timed_mutex callback_mutex_t;

private:
// Note: when LuaCallbackObject is copied or moved, control block is created from scratch
	struct ControlBlock {
		LuaServer *owner=nullptr;
		std::atomic<bool> enableUnsafeDestruction {false};
		std::thread::id unsafeDestructionThread;
		std::vector<std::thread::id> callbackThreadStack;
		callback_mutex_t *callbackMutex=nullptr;
	};
	
	std::shared_ptr<ControlBlock> _cb;
	
public:
	LuaCallbackObject();
	LuaCallbackObject(const LuaCallbackObject &);
	LuaCallbackObject(LuaCallbackObject &&);
	virtual ~LuaCallbackObject();

	LuaCallbackObject &operator=(const LuaCallbackObject &);
	LuaCallbackObject &operator=(LuaCallbackObject &&);
	
	void setCallbackMutex(callback_mutex_t *m) {_cb->callbackMutex=m;}
	callback_mutex_t *callbackMutex() const {return _cb->callbackMutex;}
	
	virtual std::string objectType() const=0;

protected:
	virtual std::function<int(LuaServer&)> enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues)=0;

// Pointer to the LuaServer managing this object's lifecycle, or nullptr if the object is unmanaged
	LuaServer *owner() const {return _cb->owner;}

// Detach from the LuaServer without deletion
	void detach();

/* 
 * LuaCallbackObject prevents object unregistration from the threads other
 * than the current Lua thread while callback is in progress. Use the following
 * function to explicitly allow unregistration from the current thread.
 */
	void enableUnsafeDestruction(bool b);

private:
	int gcDispose(LuaServer &); // default garbage collection handler
};

#endif
