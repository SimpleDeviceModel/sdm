#include "export.h"
#include "testlib.h"

#include <iostream>

class StaticTest {
	int *_ptr=nullptr;
public:
	StaticTest() {
		std::cout<<"Creating StaticTest instance"<<std::endl;
	}
	virtual ~StaticTest() {
		std::cout<<"Destroying StaticTest instance"<<std::endl;
		func1();
		if(_ptr) *_ptr=1;
	}
	void setPtr(int *ptr) {
		_ptr=ptr;
	}
};

StaticTest staticTestInstance;

EXPORT void testfunc(int *ptr) {
	std::cout<<"testfunc() invoked"<<std::endl;
	staticTestInstance.setPtr(ptr);
}
