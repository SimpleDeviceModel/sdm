#ifdef NDEBUG
	#undef NDEBUG
#endif

#include "u8ecodec.h"
#include "u8efile.h"

#include <iostream>
#include <cstdlib>
#include <cassert>

using namespace u8e;

void test1() {
	Codec codec(UTF8,ShiftJIS);
	
	IFileStream in("test.utf8.orig",std::ios::in|std::ios::binary);
	OFileStream out("test.shiftjis",std::ios::out|std::ios::binary);
	
	char buf[256];
	
	for(;;) {
		in.read(buf,1+rand()%9);
		if(!in.gcount()) break;
		out<<codec.transcode(std::string(buf,static_cast<std::size_t>(in.gcount())));
	}
	
	assert(!codec.incomplete());
}

void test2() {
	Codec codec(ShiftJIS,UTF8);
	
	IFileStream in("test.shiftjis",std::ios::in|std::ios::binary);
	OFileStream out("test.utf8",std::ios::out|std::ios::binary);
	
	char buf[256];
	
	for(;;) {
		in.read(buf,1+rand()%9);
		if(!in.gcount()) break;
		out<<codec.transcode(std::string(buf,static_cast<std::size_t>(in.gcount())));
	}
	
	assert(!codec.incomplete());
}

int main() {
	test1();
	test2();
	
	return 0;
}
