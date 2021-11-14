// Allow assertions in Release mode
#ifdef NDEBUG
	#undef NDEBUG
#endif

#include "dirutil.h"

#ifdef _WIN32
	#define DIRSEP_STR "\\"
#else
	#define DIRSEP_STR "/"
#endif

#include <iostream>
#include <string>
#include <cassert>

int testpath(const std::string &in,const std::string &expected) {
	if(Path(in).str()==expected) return 1;
	std::cout<<"Test failed"<<std::endl;
	std::cout<<"Input string was: ["<<in<<"]"<<std::endl;
	std::cout<<"Expected: "<<expected<<std::endl;
	std::cout<<"Got: "<<Path(in).str()<<std::endl;
	return 0;
}

int main(int argc,char *argv[]) {
	std::cout<<"Exe path: ["<<Path::exePath().str()<<"]"<<std::endl;
	std::cout<<"Current dir: ["<<Path::current().str()<<"]"<<std::endl;
	std::cout<<"Home dir: ["<<Path::home().str()<<"]"<<std::endl;
	std::cout<<"argv[0]="<<argv[0]<<std::endl;
	std::cout<<"Full path to argv[0]: ["<<Path(argv[0]).toAbsolute().str()<<"]"<<std::endl;
	
	std::cout<<"Test path normalization"<<std::endl;
	
	assert(testpath("/usr/local/bin",DIRSEP_STR "usr" DIRSEP_STR "local" DIRSEP_STR "bin"));
	assert(testpath("/usr/local/bin/",DIRSEP_STR "usr" DIRSEP_STR "local" DIRSEP_STR "bin"));
	assert(testpath("/usr///local//////bin//",DIRSEP_STR "usr" DIRSEP_STR "local" DIRSEP_STR "bin"));
	assert(testpath("//server/share/path",DIRSEP_STR DIRSEP_STR "server" DIRSEP_STR "share" DIRSEP_STR "path"));
	assert(testpath("//server//badshare/path///",DIRSEP_STR DIRSEP_STR "server" DIRSEP_STR "badshare" DIRSEP_STR "path"));
	assert(testpath("///server//badshare/path/",DIRSEP_STR DIRSEP_STR "server" DIRSEP_STR "badshare" DIRSEP_STR "path"));
	
	assert(testpath("../test",".." DIRSEP_STR "test"));
	assert(testpath("qwe/../test","test"));
	assert(testpath("test1/./test2/../../../test",".." DIRSEP_STR "test"));
	assert(testpath("/../test1//./test2/../foo/",DIRSEP_STR "test1" DIRSEP_STR "foo"));
	assert(testpath("1/2/3/4/../../5/./6","1" DIRSEP_STR "2" DIRSEP_STR "5" DIRSEP_STR "6"));
	assert(testpath("1/2/././","1" DIRSEP_STR "2"));
	assert(testpath("/home/wwwuser/../../etc/passwd",DIRSEP_STR "etc" DIRSEP_STR "passwd"));
	
	std::cout<<"Test path concatenation"<<std::endl;
	
	assert((Path("/usr")+"bin/bash").str()==DIRSEP_STR "usr" DIRSEP_STR "bin" DIRSEP_STR "bash");
	assert((Path("/usr")+"/bin/bash").str()==DIRSEP_STR "bin" DIRSEP_STR "bash");
	assert((Path("//server/share/path")+"filename").str()==DIRSEP_STR DIRSEP_STR "server" DIRSEP_STR "share" DIRSEP_STR "path" DIRSEP_STR "filename");
	assert((Path("/")+"..").str()==DIRSEP_STR);
	assert((Path("/")+"..//..").str()==DIRSEP_STR);
	assert((Path()+"..//..").str()==".." DIRSEP_STR "..");
	assert((Path("/usr/bin")+"../../etc/passwd").str()==DIRSEP_STR "etc" DIRSEP_STR "passwd");
	assert((Path("/usr/bin")+"/../../etc/passwd").str()==DIRSEP_STR "etc" DIRSEP_STR "passwd");
	assert((Path("/usr/bin")+"//../../etc/passwd").str()==DIRSEP_STR DIRSEP_STR "etc" DIRSEP_STR "passwd");
	assert((Path("/usr/bin")+"////../../etc/passwd").str()==DIRSEP_STR DIRSEP_STR "etc" DIRSEP_STR "passwd");
	assert((Path(".config//kde4/../gnupg")+"././../../foo").str()=="foo");
	
	std::cout<<"Test absolute path detection"<<std::endl;
	
	assert(Path("/usr").isAbsolute()==true);
	assert(Path("usr").isAbsolute()==false);
	assert(Path("//server.share").isAbsolute()==true);
	
	std::cout<<"Test path comparison"<<std::endl;
	
	assert(Path("/usr/local/bin/")==Path("/root/.local/../../etc/.//../usr/share/../local/bin/.//"));
	assert(Path("/usr/local/bin/")!=Path("usr/local/bin"));
	
	std::cout<<"Test Path::up()"<<std::endl;
	
	assert(Path("/usr/local/bin").up(0).str()==DIRSEP_STR "usr" DIRSEP_STR "local" DIRSEP_STR "bin");
	assert(Path("/usr/local/bin").up().str()==DIRSEP_STR "usr" DIRSEP_STR "local");
	assert(Path("/usr/local/bin").up(1).str()==DIRSEP_STR "usr" DIRSEP_STR "local");
	assert(Path("/usr/local/bin").up(2).str()==DIRSEP_STR "usr");
	assert(Path("/usr/local/bin").up(3).str()==DIRSEP_STR);
	assert(Path("/usr/local/bin").up(4).str()==DIRSEP_STR);
	assert(Path("/usr/local/bin").up(5).str()==DIRSEP_STR);
	
	assert(Path("usr/local/bin").up(0).str()=="usr" DIRSEP_STR "local" DIRSEP_STR "bin");
	assert(Path("usr/local/bin").up().str()=="usr" DIRSEP_STR "local");
	assert(Path("usr/local/bin").up(1).str()=="usr" DIRSEP_STR "local");
	assert(Path("usr/local/bin").up(2).str()=="usr");
	assert(Path("usr/local/bin").up(3).str()==".");
	assert(Path("usr/local/bin").up(4).str()=="..");
	assert(Path("usr/local/bin").up(5).str()==".." DIRSEP_STR "..");
	
	std::cout<<"Test Path::count()"<<std::endl;
	
	assert(Path("/usr/local/bin").count()==4);
	assert(Path("usr/local/bin").count()==3);
	assert(Path("foo").count()==1);
	assert(Path(".").count()==0);
	assert(Path("..").count()==-1);
	assert(Path("..//..").count()==-2);
	assert(Path("../foo").count()==0);
	assert(Path("../foo/bar").count()==1);
	assert(Path("../../foo").count()==-1);
	
	std::cout<<"Miscellaneous tests"<<std::endl;
	
	assert(Path().str()==".");
	assert(Path("..").str()=="..");
	
	return 0;
}
