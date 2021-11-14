#ifdef NDEBUG
	#undef NDEBUG
#endif

#include "u8ecodec.h"
#include "u8efile.h"
#include "u8eio.h"

#include <iostream>
#include <cstdlib>
#include <cassert>

using namespace u8e;

void test1() {
	utf8cout()<<"Subtest #1"<<endl;
	Codec codec(UTF8,Windows1251);
	
	IFileStream in("test.utf8.orig",std::ios::in|std::ios::binary);
	OFileStream out("test.cp1251",std::ios::out|std::ios::binary);
	
	char buf[256];
	
	for(;;) {
		in.read(buf,1+rand()%9);
		if(!in.gcount()) break;
		out<<codec.transcode(std::string(buf,static_cast<std::size_t>(in.gcount())));
	}
	
	assert(!codec.incomplete());
}

void test2() {
	utf8cout()<<"Subtest #2"<<endl;
	Codec codec(Windows1251,UTF8);
	
	IFileStream in("test.cp1251",std::ios::in|std::ios::binary);
	OFileStream out("test.utf8",std::ios::out|std::ios::binary);
	
	char buf[256];
	
	for(;;) {
		in.read(buf,1+rand()%9);
		if(!in.gcount()) break;
		out<<codec.transcode(std::string(buf,static_cast<std::size_t>(in.gcount())));
	}
	
	assert(!codec.incomplete());
}

void test3() {
	utf8cout()<<"Subtest #3"<<endl;
// Test UTF-8 auto-sync
	Codec codec(UTF8,Windows1251);
	
	const char *in="\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";
	const char *expected="\xF0\xE8\xE2\xE5\xF2";
	
	std::string res=codec.transcode(in);
	assert(res==expected);
	
	const char *in2="foo" "\xD0\x20" "bar";
	res=codec.transcode(in2);
	assert(res.find("foo")!=std::string::npos);
	assert(res.find("bar")!=std::string::npos);
}

void test4() {
	utf8cout()<<"Subtest #4"<<endl;
	Codec codec(UTF8,Windows1251);
	assert(codec.transcode("")=="");
}

void test5() {
	utf8cout()<<"Subtest #5"<<endl;
	Codec codec(UTF8,UTF16);
	codec.transcode("\xD1\x80\xD0\xB8\xD0");
	assert(codec.incomplete()==1);
	codec.reset();

	std::string s=codec.transcode("\xf0");
	assert(codec.incomplete()==1);
	assert(s.empty());
	s=codec.transcode("\x9d");
	assert(codec.incomplete()==2);
	assert(s.empty());
	s=codec.transcode("\x92");
	assert(codec.incomplete()==3);
	assert(s.empty());
	s=codec.transcode("\xb3");
	assert(codec.incomplete()==0);
}

int main() {
	test1();
	test2();
	test3();
	test4();
	test5();
	
	return 0;
}
