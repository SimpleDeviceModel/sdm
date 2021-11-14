// Enable assertions even in release builds
#ifdef NDEBUG
	#undef NDEBUG
#endif

#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include "dfic.h"

#include <iostream>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <cassert>

#ifdef _WIN32
	#define EXPORT extern "C" __declspec(dllexport)
#elif (__GNUC__>=4)
	#define EXPORT extern "C" __attribute__((__visibility__("default")))
#else
	#define EXPORT extern "C"
#endif

#ifdef DFIC_CPU_X86
	#ifdef __GNUC__
		#define FASTCALL __attribute__((fastcall))
		#define STDCALL __attribute__((stdcall))
	#elif defined(_WIN32)
		#define FASTCALL __fastcall
		#define STDCALL __stdcall
	#else
		#define FASTCALL
		#define STDCALL
	#endif
#else
	#define FASTCALL
	#define STDCALL
#endif

char buf1[1024]="Return string from library || ";

EXPORT void test1() {
	std::cout<<"Entered test1()"<<std::endl;
}

EXPORT int test2(char ch,int i) {
	std::cout<<"Entered test2()"<<std::endl;
	std::cout<<"ch="<<ch<<", i="<<i<<std::endl;
	return i+ch;
}

EXPORT double test3(double d,float f) {
	std::cout<<"Entered test3()"<<std::endl;
	std::cout<<"d="<<d<<", f="<<f<<std::endl;
	return d+f;
}

EXPORT char *test4(const char *sz) {
	std::cout<<"Entered test4()"<<std::endl;
	std::cout<<"sz="<<sz<<std::endl;
	assert(std::strlen(sz)<=256);
	std::strcat(buf1,sz);
	return buf1;
}

EXPORT int STDCALL test5(char ch,double d) {
	std::cout<<"Entered test5()"<<std::endl;
	std::cout<<"ch="<<ch<<", d="<<d<<std::endl;
	return static_cast<int>(ch+d);
}

EXPORT int test6(int x,int y) {
	static int cnt=0;
	std::cout<<"Entered test6()"<<std::endl;
	std::cout<<"x="<<x<<", y="<<y<<", cnt="<<cnt<<std::endl;
	return x*y+(cnt++);
}

// Note: to properly test passing arguments via stack using SystemV AMD64 ABI,
// this function must have >6 integral and >8 floating point parameters

EXPORT double test8(char ch1,char ch2,char ch3,char ch4,char ch5,char ch6,
		    double d1,double d2,double d3,double d4,double d5,double d6,double d7,double d8,
		    int i,float f,const char *sz,double d) {
	std::cout<<"Entered test8()"<<std::endl;
	std::cout<<"ch1="<<ch1<<std::endl;
	std::cout<<"ch2="<<ch2<<std::endl;
	std::cout<<"ch3="<<ch3<<std::endl;
	std::cout<<"ch4="<<ch4<<std::endl;
	std::cout<<"ch5="<<ch5<<std::endl;
	std::cout<<"ch6="<<ch6<<std::endl;
	std::cout<<"d1="<<d1<<std::endl;
	std::cout<<"d2="<<d2<<std::endl;
	std::cout<<"d3="<<d3<<std::endl;
	std::cout<<"d4="<<d4<<std::endl;
	std::cout<<"d5="<<d5<<std::endl;
	std::cout<<"d6="<<d6<<std::endl;
	std::cout<<"d7="<<d7<<std::endl;
	std::cout<<"d8="<<d8<<std::endl;
	std::cout<<"i="<<i<<std::endl;
	std::cout<<"f="<<f<<std::endl;
	std::cout<<"sz="<<sz<<std::endl;
	std::cout<<"d="<<d<<std::endl;
	
	double res=ch1+ch2+ch3+ch4+ch5+ch6+
		d1+d2+d3+d4+d5+d6+d7+d8+
		i+f+std::strlen(sz)+d;
	
	std::cout<<"Returning "<<res<<std::endl;
	
	return res;
}

EXPORT int test9(int i) {
	static int acc=0;
	std::cout<<"Entered test9()"<<std::endl;
	acc+=i;
	return acc;
}

EXPORT const double *test10(char *psz,int *pi,double *pd) {
	static const double d=66.25;
	
	std::cout<<"Entered test10()"<<std::endl;
	std::cout<<"psz="<<psz<<std::endl;
	for(int i=0;i<4;i++) std::cout<<"pi["<<i<<"]="<<pi[i]<<std::endl;
	std::cout<<"*pd="<<*pd<<std::endl;
	
	std::strcpy(psz,"Return string");
	pi[0]=0x12;
	pi[1]=0x34;
	pi[2]=0x56;
	pi[3]=0x78;
	*pd=-99.5;
	
	return &d;
}

EXPORT double FASTCALL test11(float f,char *sz,double d,int i) {
	std::cout<<"Entered test11()"<<std::endl;
	std::cout<<"f="<<f<<std::endl;
	std::cout<<"sz="<<sz<<std::endl;
	std::cout<<"d="<<d<<std::endl;
	std::cout<<"i="<<i<<std::endl;	
	return d+f+i+std::strlen(sz);
}

EXPORT std::uint64_t test12(std::uint64_t a,int b) {
	std::cout<<"Entered test12()"<<std::endl;
	std::cout<<"a="<<a<<std::endl;
	std::cout<<"b="<<b<<std::endl;
	return a+b;
}

EXPORT float test13(double d) {
	return static_cast<float>(d);
}
