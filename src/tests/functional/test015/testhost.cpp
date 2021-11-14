// Allow assertions in Release mode
#ifdef NDEBUG
	#undef NDEBUG
#endif

#include "loadablemodule.h"
#include "testlib.h"

#include <iostream>
#include <cassert>

typedef void (*TestFuncPtr)(int *ptr);

int isDestroyed=0;

int main(int argc,char *argv[]) {
	if(argc!=2) {
		std::cerr<<"Library name must be supplied"<<std::endl;
		return EXIT_FAILURE;
	}
	
	func1();
	
	std::cout<<"Loading the module"<<std::endl;
	LoadableModule mod(argv[1]);
	std::cout<<"Obtaining the function address"<<std::endl;
	
	TestFuncPtr func;
	try {
		func=reinterpret_cast<TestFuncPtr>(mod.getAddr("testfunc"));
	}
	catch(std::exception &) {
		func=reinterpret_cast<TestFuncPtr>(mod.getAddr("_testfunc"));
	}
	
	std::cout<<"Invoking the function"<<std::endl;
	func(&isDestroyed);
	std::cout<<"Unloading the module"<<std::endl;
	mod.unload();
	std::cout<<"Checking whether the destructor has been called..."<<std::endl;
	std::cout<<"isDestroyed="<<isDestroyed<<std::endl;
	assert(isDestroyed!=0);
	std::cout<<"Seems to be OK"<<std::endl;
}
