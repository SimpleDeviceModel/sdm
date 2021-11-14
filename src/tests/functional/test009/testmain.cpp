#ifdef NDEBUG // allow assertions even in release mode
	#undef NDEBUG
#endif

#define NOBJECTS 10000
#define NTESTS1 10
#define NTESTS2 100

#include "luaserver.h"
#include "luacallbackobject.h"

#include <iostream>
#include <fstream>
#include <cassert>
#include <stdexcept>
#include <string>
#include <deque>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>

class TestLuaObject : public LuaCallbackObject {
	std::deque<std::string> queue;
	LuaServer &lua;
	LuaValue _handle;
	
public:
	TestLuaObject(LuaServer &l): lua(l) {
		_handle=lua.registerObject(*this);
	}
	virtual ~TestLuaObject() {
		lua.unregisterObject(*this);
	}
	
	LuaValue handle() const {return _handle;}
	
	virtual std::string objectType() const {return "TestLuaObject";}
	virtual std::function<int(LuaServer&)> enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
		switch(i) {
		case 0:
			strName="qput";
			return std::bind(&TestLuaObject::qput,this,std::placeholders::_1);
		case 1:
			strName="qget";
			return std::bind(&TestLuaObject::qget,this,std::placeholders::_1);
		case 2:
			strName="waitget";
			return std::bind(&TestLuaObject::waitget,this,std::placeholders::_1);
		default:
			return std::function<int(LuaServer&)>();
		}
	}
	int qput(LuaServer &lua) {
		if(lua.argc()!=1) throw std::runtime_error("Wrong number of arguments");
		queue.push_back(lua.argv(0).toString());
		return 0;
	}
	int qget(LuaServer &lua) {
		if(lua.argc()!=0) throw std::runtime_error("Wrong number of arguments");
		lua.pushValue(queue.front());
		queue.pop_front();
		return 1;
	}
	int waitget(LuaServer &lua) {
// wait while the object is being deleted in the main thread
		std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
		return qget(lua);
	}
};

union Placeholder {
	char buf[sizeof(TestLuaObject)];
	long double alignme;
};

struct Completer {
	LuaCallResult res;
	bool completed;
	
	Completer(): completed(false) {}
	
	void completer(const LuaCallResult &res) {
		Completer::res=res;
		if(!res.success) {
			std::cout<<"Lua returned an error message: "<<res.errorMessage<<std::endl;
		}
		completed=true;
	}
};

Placeholder *buffers[NOBJECTS];
TestLuaObject *objectptrs[NOBJECTS];

void runtest1(LuaServer &lua) {
	std::string strProgram;
	std::ifstream in("runtest1.lua");
	assert(in);

// Read test program
	for(;;) {
		char buf[256];
		in.read(buf,256);
		if(!in.gcount()) break;
		strProgram.append(buf,static_cast<std::size_t>(in.gcount()));
	}

// Create and register test
	TestLuaObject *objectptr=new(buffers[0]->buf) TestLuaObject(lua);
	lua.setGlobal("obj",objectptr->handle());

// Run Lua program
	Completer c;
	lua.executeChunkAsync(strProgram,"test",std::bind(&Completer::completer,&c,std::placeholders::_1));
	
// Wait a little bit
	std::this_thread::sleep_for(std::chrono::milliseconds(50));

// Destroy object
	objectptr->~TestLuaObject();
	for(std::size_t j=0;j<sizeof(Placeholder);j++) {
		const_cast<volatile char*>(buffers[0]->buf)[j]='\xFE';
	}
	
// Wait for thread completion
	lua.wait();
	
	assert(c.completed);
	assert(c.res.success);
}

void runtest2(LuaServer &lua) {
	std::string strProgram;
	std::ifstream in("runtest2.lua");
	assert(in);

// Read test program
	for(;;) {
		char buf[256];
		in.read(buf,256);
		if(!in.gcount()) break;
		strProgram.append(buf,static_cast<std::size_t>(in.gcount()));
	}

// Create and register test objects
	LuaValue t;
	t.newtable();
	for(int i=0;i<NOBJECTS;i++) {
		objectptrs[i]=new(buffers[i]->buf) TestLuaObject(lua);
		t.table().emplace(static_cast<lua_Integer>(i+1),objectptrs[i]->handle());
	}
	
	lua.setGlobal("objects",t);

// Run Lua program
	Completer c;
	lua.executeChunkAsync(strProgram,"test",std::bind(&Completer::completer,&c,std::placeholders::_1));

// Begin to delete objects
	for(int i=NOBJECTS-1;i>=0;i--) {
		std::this_thread::yield();
		objectptrs[i]->~TestLuaObject();
		for(std::size_t j=0;j<sizeof(Placeholder);j++) {
			const_cast<volatile char*>(buffers[i]->buf)[j]='\xFE';
		}
	}
	
// Wait for thread completion
	lua.wait();
	
	assert(c.completed);
	assert(c.res.success);
}

int main() {
	LuaServer lua;

	std::cout<<"Part 1"<<std::endl;
	buffers[0]=new Placeholder;
	for(int i=0;i<NTESTS1;i++) runtest1(lua);
	delete buffers[0];
	
	std::cout<<"Part 2"<<std::endl;
	for(int i=0;i<NOBJECTS;i++) buffers[i]=new Placeholder;	
	for(int i=0;i<NTESTS2;i++) runtest2(lua);
	for(int i=0;i<NOBJECTS;i++) delete buffers[i];
	
	return 0;
}
