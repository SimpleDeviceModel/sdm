This directory contains example SDM plugins.

"simpleplugin" is a very simple one written in C. It demonstrates only basic
SDM features.

"testplugin" is a more advanced example written in C++11. It must be linked
against the "pluginprovider" library. "testplugin" tries to emulate various
hardware-related issues such as delays resulting from data being unavailable
at the moment. It is used by the SDM test suite to verify SDM base system
behavior correctness.

Note: under Microsoft Windows, some less common compilers can get confused if
the user doesn't define DllMain function, even though Microsoft considers it
optional. This situation can manifest itself in linker errors, or worse yet,
crashes when the plugin tries to invoke a runtime library function. If you
experience such problems, provide a definition of DllMain as follows:

#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	return TRUE;
}
