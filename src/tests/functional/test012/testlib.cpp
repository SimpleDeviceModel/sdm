#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include <iostream>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <string>

#ifdef _WIN32
	#define EXPORT extern "C" __declspec(dllexport)
#elif (__GNUC__>=4)
	#define EXPORT extern "C" __attribute__((__visibility__("default")))
#else
	#define EXPORT extern "C"
#endif

#if defined(__i386) || defined(_M_IX86)
	#if defined(__GNUC__)
		#define FASTCALL __attribute__((fastcall))
	#elif defined(_WIN32)
		#define FASTCALL __fastcall
	#else
		#define FASTCALL
	#endif
#else
	#define FASTCALL
#endif

EXPORT void test1() {
	std::cout<<"Entered test1()"<<std::endl;
}

EXPORT double test2(char ch,int i,double d) {
	std::cout<<"Entered test2()"<<std::endl;
	std::cout<<"ch="<<ch<<", i="<<i<<", d="<<d<<std::endl;
	return d+i+ch;
}

EXPORT const double *test3(char *s1,const char *s2,int *pi,double *pd) {
	static const double d=66.25;
	
	std::cout<<"Entered test3()"<<std::endl;
	std::cout<<"s1="<<s1<<std::endl;
	std::cout<<"s2="<<s2<<std::endl;
	for(int i=0;i<4;i++) std::cout<<"pi["<<i<<"]="<<pi[i]<<std::endl;
	std::cout<<"*pd="<<*pd<<std::endl;
	
	std::strcat(s1,s2);
	pi[0]+=1;
	pi[1]+=2;
	pi[2]+=3;
	pi[3]+=4;
	*pd-=99.5;
	
	return &d;
}

EXPORT std::uint8_t *test4(char *str) {
	std::cout<<"Entered test4()"<<std::endl;
	return reinterpret_cast<std::uint8_t*>(str);
}

EXPORT int test5(char *sz,char **other) {
	static char buf[256]="Other string";
	std::cout<<"Entered test5()"<<std::endl;
	int i=std::strtol(sz,NULL,0);
	std::strcpy(sz,std::to_string(i*2).c_str());
	*other=buf;
	return i;
}

EXPORT double FASTCALL test10(int a,float f,char *str,int b) {
	std::cout<<"Entered test10()"<<std::endl;
	return f+a+b+std::strlen(str);
}

EXPORT std::uint64_t test11(int a,std::uint64_t b) {
	std::cout<<"Entered test11()"<<std::endl;
	return a+b;
}
