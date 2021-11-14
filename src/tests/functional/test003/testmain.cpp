#include "testcommon.h"
#include "luacallbackobject.h"

#include <iostream>
#include <stdexcept>
#include <string>

class TestObject : public LuaCallbackObject {
	std::string content;
public:
	TestObject(): content("Default String") {}
	virtual std::string objectType() const {return "TestObject";}
	virtual std::function<int(LuaServer&)> enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
		switch(i) {
		case 0:
			strName="getstring";
			return std::bind(&TestObject::getstring,this,std::placeholders::_1);
		case 1:
			strName="setstring";
			return std::bind(&TestObject::setstring,this,std::placeholders::_1);
		case 2:
			strName="unregister";
			return std::bind(&TestObject::unregister,this,std::placeholders::_1);
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
	int unregister(LuaServer &lua) {
		if(lua.argc()>0) throw std::runtime_error("Wrong number of arguments");
		if(lua.nupv()>0) throw std::runtime_error("Wrong number of upvalues");
		lua.unregisterObject(*this);
		return 0;
	}
};

TestObject t1,t2;

int testmain(LuaServer &lua) {
	LuaValue t;
	
	t.newtable();
	
	t.table()["obj1"]=lua.registerObject(t1);
	t.table()["obj2"]=lua.registerObject(t2);
		
	lua.pushValue(t);
	
	return 1;
}
