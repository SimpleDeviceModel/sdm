#if defined(__i386) || defined(_M_IX86)
	#define CPU_NAME "x86"
#elif defined(__x86_64__) || defined(_M_X64)
	#define CPU_NAME "x64"
#elif defined(__arm__) || defined(_M_ARM)
	#define CPU_NAME "arm"
#else
	#define CPU_NAME "unknown"
#endif

#include <stdio.h>

int main(void) {
	printf("DetectedCPU=" CPU_NAME);
	return 0;
}
