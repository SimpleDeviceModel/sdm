// Enable assertions even in release builds
#ifdef NDEBUG
	#undef NDEBUG
#endif

#include "dfic.h"
#include "loadablemodule.h"

#include <iostream>
#include <cassert>
#include <cstring>
#include <exception>
#include <cstdlib>

static void test1();
static void test2();
static void test3();
static void test4();
static void test5();
static void test6();
static void test7();
static void test8();
static void test9();
static void test10();
static void test11();
static void test12();
static void test13();
static void test14();

LoadableModule testlib;

int main(int argc,char *argv[]) {
	if(argc!=2) {
		std::cerr<<"Library name must be supplied"<<std::endl;
		return EXIT_FAILURE;
	}
	
	testlib.load(argv[1]);
	
	test1();
	test2();
	test3();
	test4();
	test5();
	test6();
	test7();
	test8();
	test9();
	test10();
	test11();
	test12();
	test13();
	test14();
	
	return 0;
}

void test1() {
	std::cout<<"#1: Basic sanity check: call function without arguments or return value"<<std::endl;
	
	Dfic::Interface dfic(testlib.getAddr("test1"));
	dfic.invoke();
}

void test2() {
	std::cout<<"#2: Basic parameter and return value passing"<<std::endl;
	
	Dfic::Interface dfic(testlib.getAddr("test2"));
	dfic.setArgument<char>('g');
	dfic.setArgument<int>(-2000);
	dfic.invoke();
	assert(dfic.returnValue<int>()==-2000+'g');
}

void test3() {
	std::cout<<"#3: Floating point test"<<std::endl;
	
	Dfic::Interface dfic(testlib.getAddr("test3"));
	dfic.setArgument<double>(5.25);
	dfic.setArgument<float>(-4.75f);
	dfic.invoke();
	assert(dfic.returnValue<double>()==0.5);
}

void test4() {
	std::cout<<"#4: Pointer test"<<std::endl;
	
	Dfic::Interface dfic(testlib.getAddr("test4"));
	dfic.setArgument<const char*>("Test string");
	dfic.invoke();
	char *res=dfic.returnValue<char*>();
	std::cout<<"Returned string: "<<res<<std::endl;
	assert(std::strstr(res,"from library"));
}

void test5() {
	std::cout<<"#5: Test stdcall calling convention (if supported)"<<std::endl;
	
	Dfic::GenericFuncPtr f=nullptr;
	
	for(auto const &sym: {"test5","_test5","test5@12","_test5@12"}) {
		try {
			f=testlib.getAddr(sym);
			break;
		}
		catch(std::exception &) {}
	}
	
	assert(f);
	
	Dfic::Interface dfic(f);
	dfic.setArgument<char>('0');
	dfic.setArgument<double>(1.5);
#if defined(DFIC_CPU_X86) && (defined(_WIN32) || defined(__GNUC__))
	dfic.setCallingConvention(Dfic::StdCall);
#endif
	dfic.invoke();
	assert(dfic.returnValue<int>()==0x31);
}

void test6() {
	std::cout<<"#6: Invoke function multiple times"<<std::endl;
	
	Dfic::Interface dfic(testlib.getAddr("test6"));
	dfic.setArgument<int>(2);
	dfic.setArgument<int>(3);
	dfic.invoke();
	assert(dfic.returnValue<int>()==6);
	dfic.setArgument<int>(0,10);
	dfic.setArgument<int>(1,100);
	dfic.invoke();
	assert(dfic.returnValue<int>()==1001);
}

void test7() {
#ifdef _WIN32
	std::cout<<"#7: Invoke functions from Windows DLL"<<std::endl;
	
	LoadableModule advapi32("advapi32.dll");
	
	void *key;
	
	Dfic::Interface dfic(advapi32.getAddr("RegOpenKeyExA"));
#ifdef DFIC_CPU_X86
	dfic.setCallingConvention(Dfic::StdCall);
#endif
	dfic.setArgument<void*>(reinterpret_cast<void*>(static_cast<Dfic::MachineWord>(0x80000002))); // HKEY_LOCAL_MACHINE
	dfic.setArgument<const char*>("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");
	dfic.setArgument<std::uint32_t>(0);
	dfic.setArgument<std::uint32_t>(0x20019); // KEY_READ
	dfic.setArgument<void*>(&key);
	dfic.invoke();
	assert(dfic.returnValue<int>()==0);
	
	char buf[1024];
	std::uint32_t bufSize=1024;
	
	dfic.reset();
	dfic.setFunctionPointer(advapi32.getAddr("RegQueryValueExA"));
#ifdef DFIC_CPU_X86
	dfic.setCallingConvention(Dfic::StdCall);
#endif
	dfic.setArgument<void*>(key);
	dfic.setArgument<const char*>("ProductName");
	dfic.setArgument<void*>(NULL);
	dfic.setArgument<void*>(NULL);
	dfic.setArgument<char*>(buf);
	dfic.setArgument<std::uint32_t*>(&bufSize);
	dfic.invoke();
	assert(dfic.returnValue<int>()==0);
	assert(bufSize==std::strlen(buf)+1);
	std::cout<<"Windows version: "<<buf<<std::endl;
	
	dfic.reset();
	dfic.setFunctionPointer(advapi32.getAddr("RegCloseKey"));
#ifdef DFIC_CPU_X86
	dfic.setCallingConvention(Dfic::StdCall);
#endif
	dfic.setArgument<void*>(key);
	dfic.invoke();
	assert(dfic.returnValue<int>()==0);
#else
	std::cout<<"#7: Invoke functions from Windows DLL -- skipped"<<std::endl;
#endif
}

void test8() {
	std::cout<<"#8: Function with many parameters"<<std::endl;
	
	Dfic::Interface dfic(testlib.getAddr("test8"));
	dfic.setArgument<char>('a');
	dfic.setArgument<char>('b');
	dfic.setArgument<char>('c');
	dfic.setArgument<char>('d');
	dfic.setArgument<char>('e');
	dfic.setArgument<char>('f');
	dfic.setArgument<double>(0.5);
	dfic.setArgument<double>(1);
	dfic.setArgument<double>(1.5);
	dfic.setArgument<double>(2);
	dfic.setArgument<double>(2.5);
	dfic.setArgument<double>(3);
	dfic.setArgument<double>(3.5);
	dfic.setArgument<double>(4);
	dfic.setArgument<int>(1000);
	dfic.setArgument<float>(-0.5);
	dfic.setArgument<const char*>("Qwerty");
	dfic.setArgument<double>(-1.5);
	dfic.invoke();
	assert(dfic.returnValue<double>()==1619);
}

void test9() {
	std::cout<<"#9: Test Interface object copy and move"<<std::endl;
	
	Dfic::Interface dfic(testlib.getAddr("test9"));
	dfic.setArgument<int>(5);
	dfic.invoke();
	assert(dfic.returnValue<int>()==5);
	
	Dfic::Interface dfic2(dfic);
	dfic2.invoke(); // argument should be already set
	assert(dfic2.returnValue<int>()==10);
	dfic2.setArgument(0,1);
	
	Dfic::Interface dfic3;
	dfic3=dfic2;
	dfic3.invoke();
	assert(dfic3.returnValue<int>()==11);
	dfic2.invoke();
	assert(dfic2.returnValue<int>()==12);
	dfic.invoke();
	assert(dfic.returnValue<int>()==17);
	
	Dfic::Interface dfic4(std::move(dfic));
	dfic4.invoke();
	assert(dfic4.returnValue<int>()==22);
	
	dfic4=std::move(dfic2);
	dfic4.invoke();
	assert(dfic4.returnValue<int>()==23);
}

void test10() {
	std::cout<<"#10: Pass parameters back and forth via pointers"<<std::endl;
	
	char str[256]="Argument string";
	int arr[4]={-1,-2,-3,-4};
	double d=1.5;
	
	Dfic::Interface dfic(testlib.getAddr("test10"));
	dfic.setArgument(str);
	dfic.setArgument(arr);
	dfic.setArgument(&d);
	dfic.invoke();
	
	assert(std::string(str)=="Return string");
	assert(arr[0]==0x12);
	assert(arr[1]==0x34);
	assert(arr[2]==0x56);
	assert(arr[3]==0x78);
	assert(d==-99.5);
	
	assert(*dfic.returnValue<double*>()==66.25);
}

void test11() {
	std::cout<<"#11: test fastcall calling convention (if supported)"<<std::endl;
	
	Dfic::GenericFuncPtr f=nullptr;
	
	for(auto const &sym: {"test11","_test11","@test11@20"}) {
		try {
			f=testlib.getAddr(sym);
			break;
		}
		catch(std::exception &) {}
	}
	
	assert(f);
	
	Dfic::Interface dfic(f);
#if defined(DFIC_CPU_X86) && (defined(_WIN32) || defined(__GNUC__))
	dfic.setCallingConvention(Dfic::FastCall);
#endif
	dfic.setArgument(0.5f);
	dfic.setArgument("Fastcall test");
	dfic.setArgument(1.5);
	dfic.setArgument(99);
	dfic.invoke();
	assert(dfic.returnValue<double>()==114);
}

void test12() {
	std::cout<<"#12: test returning uint64_t (especially for x86)"<<std::endl;
	
	Dfic::Interface dfic(testlib.getAddr("test12"));
	dfic.setArgument(UINT64_C(1099511627776));
	dfic.setArgument(1);
	dfic.invoke();
	
	assert(dfic.returnValue<std::uint32_t>()==1);
	assert(dfic.returnValue<std::uint64_t>()==1099511627777);
}

void test13() {
	std::cout<<"#13: test returning single-precision floating point value (especially for x64)"<<std::endl;
	
	Dfic::Interface dfic(testlib.getAddr("test13"));
	dfic.setArgument(1.5);
	dfic.invoke();
	
	assert(dfic.returnValue<float>()==1.5f);
}

void test14() {
	std::cout<<"#14: trying to import function from the loaded module without specifying module name"<<std::endl;
	
	LoadableModule mod2; // don't specify module name - it is already loaded
	
	Dfic::Interface dfic(mod2.getAddr("test2"));
	dfic.setArgument<char>('g');
	dfic.setArgument<int>(-2000);
	dfic.invoke();
	assert(dfic.returnValue<int>()==-2000+'g');
}
