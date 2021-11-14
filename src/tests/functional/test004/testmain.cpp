#include "testcommon.h"
#include "luacallbackobject.h"

#include <iostream>
#include <stdexcept>
#include <string>

using namespace std::placeholders;

int refcount=0;

class TestManagedObject : public LuaCallbackObject {
	std::string content;
	
	TestManagedObject(const TestManagedObject &orig);
	TestManagedObject &operator=(const TestManagedObject &);
public:
	TestManagedObject(): content("Default String") {refcount++;}
	virtual ~TestManagedObject() {refcount--;}
	
	virtual std::string objectType() const {return "TestManagedObject";}
	virtual std::function<int(LuaServer&)> enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
		switch(i) {
		case 0:
			strName="getstring";
			return std::bind(&TestManagedObject::getstring,this,_1);
		case 1:
			strName="setstring";
			return std::bind(&TestManagedObject::setstring,this,_1);
		case 2:
			strName="close";
			return std::bind(&TestManagedObject::close,this,_1);
		default:
			return std::function<int(LuaServer&)>();
		}
	}
	int getstring(LuaServer &lua) {
		if(lua.argc()>0) throw std::runtime_error("Wrong number of arguments");
		if(lua.nupv()>0) throw std::runtime_error("Wrong number of upvalues");
		lua.pushValue(content);
		return 1;
	}
	int setstring(LuaServer &lua) {
		if(lua.argc()!=1) throw std::runtime_error("Wrong number of arguments");
		if(lua.nupv()>0) throw std::runtime_error("Wrong number of upvalues");
		if(lua.argt(0)!=LuaValue::String) throw std::runtime_error("Wrong argument type");
		content=lua.argv(0).toString();
		return 0;
	}
	int close(LuaServer &lua) {
		if(lua.argc()>0) throw std::runtime_error("Wrong number of arguments");
		if(lua.nupv()>0) throw std::runtime_error("Wrong number of upvalues");
		delete this;
		return 0;
	}
};

class ControlObject : public LuaCallbackObject {
public:
	virtual std::string objectType() const {return "ControlObject";}
	virtual std::function<int(LuaServer&)> enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
		switch(i) {
		case 0:
			strName="makeDynamicObject";
			return std::bind(&ControlObject::makeDynamicObject,this,_1);
		case 1:
			strName="refcount";
			return std::bind(&ControlObject::refcount,this,_1);
		default:
			return std::function<int(LuaServer&)>();
		}
	}
	int makeDynamicObject(LuaServer &lua) {
		if(lua.argc()>0) throw std::runtime_error("Wrong number of arguments");
		if(lua.nupv()>0) throw std::runtime_error("Wrong number of upvalues");
		lua.pushValue(lua.addManagedObject(new TestManagedObject));
		return 1;
	}
	int refcount(LuaServer &lua) {
		if(lua.argc()>0) throw std::runtime_error("Wrong number of arguments");
		if(lua.nupv()>0) throw std::runtime_error("Wrong number of upvalues");
		lua.pushValue(lua_Integer(::refcount));
		return 1;
	}
};

ControlObject co;

int testmain(LuaServer &lua) {
	lua.pushValue(lua.registerObject(co));
	
	TestManagedObject *p=new TestManagedObject;
	LuaValue v=lua.addManagedObject(p);
	lua.setGlobal("deleted",v);
	delete p; // should be detached from Lua automatically
	
	return 1;
}
