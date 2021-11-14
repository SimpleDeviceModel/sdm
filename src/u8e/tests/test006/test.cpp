#ifdef NDEBUG
	#undef NDEBUG
#endif

#include "u8eenv.h"
#include "u8eio.h"

#include <string>
#include <cassert>
#include <cstring>

using namespace u8e;

int main(int argc,char *argv[]) {
	std::vector<std::string> args=cmdArgs(argc,argv);
	
	utf8cout()<<"Количество аргументов: "<<argc<<endl;
	
	for(std::size_t i=0;i<args.size();i++) {
		utf8cout()<<"Аргумент "<<i<<": "<<args[i]<<endl;
	}
	
// Does the local encoding support Cyrillic?
	bool supportCyrillic=false;
	Codec toLocal(UTF8,LocalMB);
	Codec fromLocal(LocalMB,UTF8);
	const std::string str="ПриветМир";
	if(fromLocal.transcode(toLocal.transcode(str))==str) supportCyrillic=true;
	
// Don't use Cyrillic letters if the local encoding doesn't support them
	std::string varName,varValue;
	if(supportCyrillic) {
		utf8cout()<<"Cyrillic is supported"<<std::endl;
		varName="Привет";
		varValue="Мир";
	}
	else {
		utf8cout()<<"Cyrillic is NOT supported"<<std::endl;
		varName="Hello";
		varValue="World";
	}
	
	setEnvVar(varName,varValue);
	assert(envVar(varName)==varValue);
	delEnvVar(varName);
	assert(envVar(varName)=="");
	
	return 0;
}
