#ifdef NDEBUG
	#undef NDEBUG
#endif

#include "u8eio.h"

#include <string>
#include <cstdlib>
#include <cassert>

using namespace u8e;

int main() {
	utf8cout()<<"Привет, мир!"<<endl;
	
	std::string line;
	std::getline(utf8cin(),line);
	
	utf8cout()<<"Вы ввели: "<<line<<endl;
	
	return 0;
}
