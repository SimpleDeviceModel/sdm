cmake_minimum_required(VERSION 3.3.0)

# DFIC currently supports only x86 and x86-64

if(TARGET_MACHINE STREQUAL "x86" OR TARGET_MACHINE STREQUAL "x64")
	add_subdirectory(luadfic)
endif()

add_subdirectory(luaipsockets)
add_subdirectory(luart)
add_subdirectory(native)

enable_testing()
