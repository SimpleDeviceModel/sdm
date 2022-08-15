# Sets up version

cmake_minimum_required(VERSION 3.3.0)

set(PRODUCT_VERSION_MAJOR 1)
set(PRODUCT_VERSION_MINOR 0)
set(PRODUCT_VERSION_PATCH 5)

try_compile(CPU_DETECTED "${CMAKE_CURRENT_BINARY_DIR}/try_compile/DetectCPU" "${CMAKE_CURRENT_LIST_DIR}/DetectCPU.c" COPY_FILE "${CMAKE_CURRENT_BINARY_DIR}/try_compile/DetectCPU.bin")

if(CPU_DETECTED)
	file(STRINGS "${CMAKE_CURRENT_BINARY_DIR}/try_compile/DetectCPU.bin" DETECT_CPU_STRINGS REGEX "DetectedCPU=[A-Za-z0-9]+")
	string(REGEX MATCH "DetectedCPU=([A-Za-z0-9]+)" DETECTED_CPU ${DETECT_CPU_STRINGS})
	set(TARGET_MACHINE ${CMAKE_MATCH_1})
	message("Target machine: ${TARGET_MACHINE}")
endif()

# If CPU detection failed for whatever reason

if(NOT TARGET_MACHINE)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(TARGET_MACHINE "x64")
	else()
		set(TARGET_MACHINE "x86")
	endif()
	message("Cannot reliably determine target machine type, assuming ${TARGET_MACHINE}")
endif()
