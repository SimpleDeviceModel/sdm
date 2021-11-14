#include "testcommon.h"

#include <iostream>
#include <stdexcept>

// Test value passing from C++ to Lua

std::vector<LuaValue> tv;
LuaValue callback1;

int test1(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("Wrong number of arguments");
	if(lua.nupv()>0) throw std::runtime_error("Wrong number of upvalues");
		
	tv.push_back(LuaValue()); // nil
	tv.push_back(true); // boolean
	tv.push_back(-98.2); // number
	tv.push_back(lua_Integer(15485941)); // integer
	tv.push_back("The quick brown fox jumps over the lazy dog"); // string
	
	LuaValue t,t1;
	
	t1.newtable();
	t1.table()[lua_Integer(1000)]="thousand";
	t1.table()[lua_Integer(1001)]="thousand one";
	
	t.newtable();
	t.table()[lua_Integer(1)]="one";
	t.table()[1.5]="one and a half";
	t.table()["two"]=lua_Integer(2);
	t.table()["nested"]=t1;
	
	tv.push_back(t); // table
	tv.push_back(LuaValue(luaopen_testlib)); // CFunction
	tv.push_back((void *)&lua); // lightuserdata
	
	for(auto it=tv.cbegin();it!=tv.cend();it++) {
		lua.pushValue(*it);
	}
	
	return static_cast<int>(tv.size());
}

// Test value passing from Lua to C++

int test2(LuaServer &lua) {
	if(lua.argc()!=(int)tv.size()) throw std::runtime_error("Wrong number of arguments");
	if(lua.nupv()>0) throw std::runtime_error("Wrong number of upvalues");
	
	for(int i=0;i<static_cast<int>(tv.size());i++) {
		if(lua.argv(i)!=tv[i]) {
			std::cout<<"Wrong value of argument "<<i<<": "<<tv[i].toString()<<" expected, got "<<lua.argv(i).toString()<<std::endl;
			throw std::runtime_error("Wrong value");
		}
	}
	
	return 0;
}

// Check that global environment can be pulled without infinite recursion

int traverse_table(const LuaValue &t) {
	int i=1; // account for top-level node
	
	if(t.type()!=LuaValue::Table) throw std::runtime_error("Wrong value type");
	
	for(auto it=t.table().cbegin();it!=t.table().cend();it++) {
		if(it->first.type()==LuaValue::Table) i+=traverse_table(it->first);
		else i++;
		if(it->second.type()==LuaValue::Table) i+=traverse_table(it->second);
		else i++;
	}
	
	return i;
}

int test3(LuaServer &lua) {
	LuaValue ge=lua.getGlobal("_G");
	
	lua.pushValue(lua_Integer(traverse_table(ge)));
	
	return 1;
}

// Test upvalues

int test4(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("Wrong number of arguments");
	if(lua.nupv()!=2) throw std::runtime_error("Wrong number of upvalues");
	
	if(lua.upvalues(0)!="qwerty") throw std::runtime_error("Wrong upvalue");
	if(lua.upvalues(1)!=lua_Integer(123)) throw std::runtime_error("Wrong upvalue");
	
	return 0;
}

// Test LuaValue::Array pseudo-type

int test5(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("Wrong number of arguments");
	if(lua.nupv()>0) throw std::runtime_error("Wrong number of upvalues");
	
	LuaValue t;
	t.newarray();
	t.array().push_back(static_cast<lua_Integer>(10));
	t.array().push_back(static_cast<lua_Integer>(115));
	
	lua.pushValue(t);
	return 1;
}

// Test unregistration

int test6(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("Wrong number of arguments");
	if(lua.nupv()>0) throw std::runtime_error("Wrong number of upvalues");
	
	lua.unregisterCallback(callback1);
	
	return 0;
}

int testmain(LuaServer &lua) {
	LuaValue t;
	std::vector<LuaValue> upv;
	
	upv.push_back("qwerty");
	upv.push_back(lua_Integer(123));
	
	t.newtable();
	
	callback1=lua.registerCallback(test1);
	t.table()["test1"]=callback1;
	t.table()["test2"]=lua.registerCallback(test2);
	t.table()["test3"]=lua.registerCallback(test3);
	t.table()["test4"]=lua.registerCallback(test4,upv);
	t.table()["test5"]=lua.registerCallback(test5);
	t.table()["test6"]=lua.registerCallback(test6);
	
	lua.pushValue(t);
	
	return 1;
}
