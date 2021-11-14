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
	Codec codec(UTF8,UTF16LE);
	
	IFileStream in("test.utf8.orig",std::ios::in|std::ios::binary);
	OFileStream out("test.utf16",std::ios::out|std::ios::binary);
	
	char buf[256];
	bool deleteBOM=true;
	
	for(;;) {
		in.read(buf,1+rand()%9);
		if(!in.gcount()) break;
		std::string utf16str=codec.transcode(std::string(buf,static_cast<std::size_t>(in.gcount())));
		if(utf16str.size()>1&&deleteBOM) {
			if(utf16str.substr(0,2)=="\xFF\xFE") utf16str.erase(0,2);
			deleteBOM=false;
		}
		out<<utf16str;
	}
	
	assert(!codec.incomplete());
}

void test2() {
	Codec codec(UTF16LE,UTF8);
	
	IFileStream in("test.utf16",std::ios::in|std::ios::binary);
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
	Codec codecle(UTF8,UTF16LE);
	std::string str=codecle.transcode("test");
	utf8cout()<<str;
	assert(str==std::string("t\0e\0s\0t\0",8));
	
	Codec codecbe(UTF8,UTF16BE);
	str=codecbe.transcode("test");
	assert(str==std::string("\0t\0e\0s\0t",8));
	
	Codec codec1(UTF16LE,UTF16LE);
	assert(codec1.transcode(std::string("t\0e\0s\0t\0",8))==std::string("t\0e\0s\0t\0",8));
	
	Codec codec2(UTF16LE,UTF16BE);
	assert(codec2.transcode(std::string("t\0e\0s\0t\0",8))==std::string("\0t\0e\0s\0t",8));
	
	Codec codec3(UTF16BE,UTF16LE);
	assert(codec3.transcode(std::string("\0t\0e\0s\0t",8))==std::string("t\0e\0s\0t\0",8));
	
	Codec codec4(UTF16BE,UTF16BE);
	assert(codec4.transcode(std::string("\0t\0e\0s\0t",8))==std::string("\0t\0e\0s\0t",8));
}

int main() {
	test1();
	test2();
	test3();
	
	return 0;
}
