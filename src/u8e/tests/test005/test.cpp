#ifdef NDEBUG
	#undef NDEBUG
#endif

#include "u8efile.h"

#include <string>
#include <cassert>
#include <cstring>

using namespace u8e;

const char *filename="æöðþ_άζω_љћџ.txt";

int main() {
// Test output
	OFileStream out(filename);
	assert(out);
	out<<"Hello, world!"<<std::endl;
	out.seekp(4,std::ios::beg);
	out<<"Foo bar";
	out.close();

// Test input
	IFileStream in(filename);
	assert(in);
	std::string str;
	in.seekg(5,std::ios::beg);
	std::getline(in,str);
	assert(str=="oo bard!");
	
// Test bidirectional I/O
	FileStream io(filename);
	assert(io);
	io<<"The quick brown fox jumps over the lazy dog"<<std::flush;
	io.seekp(-3,std::ios::cur);
	io<<"cat";
	io.seekg(-8,std::ios::end);
	std::getline(io,str);
	assert(str=="lazy cat");
	io.clear();
	io.seekg(0,std::ios::beg);
	char buf[16];
	io.read(buf,16);
	assert(io.gcount()==16);
	assert(!memcmp(buf,"The quick brown ",16));
	
	return 0;
}
