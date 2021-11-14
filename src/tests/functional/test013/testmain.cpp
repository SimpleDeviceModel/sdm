// Enable assertions even in Release builds
#ifdef NDEBUG
	#undef NDEBUG
#endif

#include "testcommon.h"
#include "luaiterator.h"

#include <iostream>
#include <stdexcept>
#include <cassert>
#include <algorithm>

void traverse(const LuaIterator &iterator,std::size_t tabs) {
	std::string indent(tabs,'\t');
	LuaIterator itCopy=iterator;
	LuaValue firstKey=itCopy.key();
	for(auto it=iterator;it;++it) {
		if(it.keyType()!=LuaValue::Table) {
			std::cout<<"Key: "<<it.key().toString()<<std::endl;
		}
		else {
			std::cout<<"Traversing key subtable..."<<std::endl;
			traverse(it.keyIterator(),tabs+1);
			std::cout<<"Subtable traversal finished"<<std::endl;
		}
		if(it.valueType()!=LuaValue::Table) std::cout<<"Value: "<<it->second.toString()<<std::endl;
		else {
			std::cout<<"Traversing value subtable..."<<std::endl;
			traverse(it.valueIterator(),tabs+1);
			std::cout<<"Subtable traversal finished"<<std::endl;
		}
		
		if(it.keyType()!=LuaValue::Table&&it.key().toString()=="changeme") {
			it.setValue("done!");
			assert(it->second.toString()=="done!");
		}
	}
	assert(itCopy.key()==firstKey);
}

int test1(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("Wrong number of arguments");
	traverse(lua.getIterator(-1),0);
	std::cout<<"lua.argc()=="<<lua.argc()<<std::endl;
	assert(lua.argc()==1);
	return 0;
}

int test2(LuaServer &lua) {
	if(lua.argc()!=0) throw std::runtime_error("Wrong number of arguments");
	
	auto it=lua.getGlobalIterator("t2");
	it=it.valueIterator(55.5);
	it=it.valueIterator(static_cast<lua_Integer>(2));
	
	lua.pushValue(it->second.toString()+"_test");
	
	assert(lua.argc()==1);
	
	return 1;
}

int test3(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("Wrong number of arguments");
	
	auto it=lua.getIterator(-1);
	
// Count items with values >=50 (converted to number)
	auto count=std::count_if(it,LuaIterator(),[](const std::pair<LuaValue,LuaValue> &item) {
		return (item.second.toNumber()>=50);
	});

// Find max element
	auto maxel=std::max_element(it,LuaIterator(),[](const std::pair<LuaValue,LuaValue> &first,
		const std::pair<LuaValue,LuaValue> &second)
	{
		return (first.second.toNumber()<second.second.toNumber());
	});
	
	lua.pushValue(static_cast<lua_Integer>(count));
	lua.pushValue(maxel->first);
	lua.pushValue(maxel->second);
	
	assert(lua.argc()==4);
	
	return 3;
}

int test4(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("Wrong number of arguments");
	
	auto name=lua.argv(0).toString();
	
	auto it=lua.getGlobalIterator(name);
	it.setValue("123456");
	
	assert(lua.argc()==1);
	
	return 0;
}

int testmain(LuaServer &lua) {
	LuaValue t;
	
	t.newtable();
	
	t.table()["test1"]=lua.registerCallback(test1);
	t.table()["test2"]=lua.registerCallback(test2);
	t.table()["test3"]=lua.registerCallback(test3);
	t.table()["test4"]=lua.registerCallback(test4);
	
	lua.pushValue(t);
	
	return 1;
}
