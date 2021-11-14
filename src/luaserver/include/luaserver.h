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
 * LuaServer is a small wrapper library integrating Lua interpreter
 * into a C++ program.
 *
 * This header file defines the main LuaServer class.
 * 
 * Thread safety:
 * 
 * LuaServer is reentrant, but not thread-safe. An instance must not
 * be accessed from multiple threads simultaneously (asynchronously
 * invoked Lua interpreter counts as a separate thread). In particular,
 * any attempt to invoke member functions working with the Lua state
 * (e.g. stack-related members) while Lua interpreter is running
 * asynchronously will very likely result in a crash.
 * 
 * As an exception, the following public members related to callback
 * registration are thread-safe:
 * 	* registerCallback()
 * 	* unregisterCallback()
 * 	* registerObject()
 * 	* unregisterObject()
 * 	* addManagedObject()
 *      * detachManagedObject()
 * 
 * In addition, LuaServer locks object's unregistration while a callback
 * is being executed on this object in another thread. This means that
 * a LuaCallbackObject instance can be safely destroyed from any thread
 * provided that is is unregistered first.
 */

#ifndef LUASERVER_H_INCLUDED
#define LUASERVER_H_INCLUDED

#include "luavalue.h"
#include "luacallbackobject.h"
#include "luastreamreader.h"

#include <memory>
#include <mutex>
#include <thread>
#include <set>
#include <atomic>
#include <functional>
#include <condition_variable>

class LuaServer;
class LuaIterator;

typedef int (*typeLuaCallback)(LuaServer &);

struct LuaCallResult {
	bool success=false;
	bool incomplete=false;
	std::vector<LuaValue> results;
	std::string errorMessage;
};

class LuaServer {
	friend class LuaIterator;
public:
	typedef std::function<void(const LuaCallResult &)> Completer;

private:
// Private data types
	struct LuaDispatchData {
		enum DispatchType {function,object};
		std::function<int(LuaServer&)> invoker;
		LuaCallbackObject *pobj;
		typeLuaCallback pfunc;
		DispatchType type;
	};
	
	struct LuaStackInfo {
		std::string source;
		int line;
	};
	
	struct Registry {
		typedef std::recursive_mutex mutex_t;
		typedef std::unique_lock<mutex_t> lock_t;
		
		mutex_t m;
		std::condition_variable_any cv;

		std::map<lua_Integer,LuaDispatchData> dispatchRecords;
		std::set<LuaCallbackObject*> managedObjects;
		std::multimap<LuaCallbackObject*,lua_Integer> registeredObjects;
		lua_Integer lastUniqueId;
		
		Registry(): lastUniqueId(0) {}
		~Registry();
		void clear();
	};
	
	struct CVPackage {
		std::mutex m;
		std::condition_variable cv;
		
		std::unique_lock<std::mutex> getLock() {
			return std::unique_lock<std::mutex>(m);
		}
		void wait(std::unique_lock<std::mutex> &lock) {
			cv.wait(lock);
		}
		void notify() {
			cv.notify_all();
		}
	};

// Data members
	lua_State *_lua;
	bool _ownState; // does the current object own the state or was it attached?
	
	std::thread _thread; // worker thread object
	std::atomic<bool> _running {false}; // is task being executed?
	CVPackage _runningCV;
	std::atomic<bool> _terminationRequested {false}; // did the user request termination?
	std::multiset<LuaCallbackObject::callback_mutex_t*> _lockedMutexes; // pointers to callback mutexes currently locked
	
	Completer _task;
	CVPackage _taskCV;
	std::atomic<bool> _finishWorkerThread {false};
	
	Registry _reg; // callback registry
	std::set<const void*> _traversedtables;
	std::vector<std::vector<std::function<void()> > > _finalizers;
	bool _autoClearStack=true;

public:
// Constructors and destructor
	LuaServer();
	LuaServer(lua_State *L);
	virtual ~LuaServer();
	
	LuaServer(const LuaServer &)=delete;
	LuaServer &operator=(const LuaServer &)=delete;

// Members related to LuaServer configuration
	void attach(lua_State *L);

// Members related to job start and termination
	LuaCallResult executeChunk(const std::string &strChunk,const std::string &strName);
	LuaCallResult executeChunk(LuaStreamReader &reader,const std::string &strName);
	void executeChunkAsync(const std::string &strChunk,const std::string &strName,const Completer &completer);
	void executeChunkAsync(LuaStreamReader &reader,const std::string &strName,const Completer &completer);
	bool busy() const;
	void terminate();
	void wait();

// Members to work with Lua stack	
	void pushValue(const LuaValue &val);
	LuaValue pullValue(int stackpos,bool tableAsArray=false);
	LuaValue popValue(bool tableAsArray=false);
	LuaValue::Type valueType(int stackPos); // obtain value type without copying
	LuaIterator getIterator(int stackPos,const LuaValue &firstKey=LuaValue());
	bool isValidIndex(int i);
	void clearstack();
	
	bool autoClearStack() const {return _autoClearStack;}
	void setAutoClearStack(bool b) {_autoClearStack=b;}

// Members to set/get Lua global variables
	void setGlobal(const std::string &name,const LuaValue &val);
	LuaValue getGlobal(const std::string &name);
	LuaIterator getGlobalIterator(const LuaValue &firstKey=LuaValue());

// Members used by callbacks to access arguments and upvalues
// Note: arguments are counted from 0, that is, "i" is different from
// the Lua stack index
	int argc(); // argument count
	LuaValue::Type argt(int i); // get argument type without copying
	LuaValue argv(int i,bool tableAsArray=false);
	int nupv();
	LuaValue upvalues(int i);

// Debug and status information
	std::string currentChunkName();
	bool checkRuntime();

// Template function to set up finalizers (to be called from callbacks)
	template <typename F> void addFinalizer(F &&functor) {
		_finalizers.back().emplace_back(std::forward<F>(functor));
	}

// Members to control callback registry
// Note: these functions are thread-safe
	LuaValue registerCallback(typeLuaCallback ptr,const std::vector<LuaValue> &upvalues=std::vector<LuaValue>());
	void unregisterCallback(const LuaValue &cb);
	
	LuaValue registerObject(LuaCallbackObject &obj);
	void unregisterObject(LuaCallbackObject &obj);
	
	LuaValue addManagedObject(LuaCallbackObject *newObj);
	void detachManagedObject(LuaCallbackObject *pObj); // detach managed object, but not destroy

protected:
	lua_State *state() const {return _lua;}
	virtual LuaCallResult execute();

private:
// Static member functions (used only internally)
	static int stackGuard(lua_State *L);
	static int globalDispatcher(lua_State *L);
	static void hookStackGuard(lua_State *L,lua_Debug *ar);
	static int globalHook(lua_State *L,lua_Debug *ar);
	static int closeFileHandle(lua_State *L);

// Non-static member functions used only internally
	LuaCallResult loadChunk(const std::string &strChunk,const std::string &strName);
	LuaCallResult loadChunk(LuaStreamReader &reader,const std::string &strName);
	void threadProc();
	
	std::vector<LuaValue> auxvalues(lua_Integer id,bool grace);
	lua_Integer makeUniqueId();
	void executeFinalizers();
};

#endif
