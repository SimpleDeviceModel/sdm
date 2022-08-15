cmake_minimum_required(VERSION 3.3.0)

if(OPTION_VALGRIND)
	set(VALGRIND "valgrind" "--leak-check=yes" "--error-exitcode=1")
endif()

add_subdirectory(test001)
add_subdirectory(test002)
add_subdirectory(test003)
add_subdirectory(test004)
add_subdirectory(test005)
add_subdirectory(test008)
add_subdirectory(test009)
add_subdirectory(test010)
add_subdirectory(test011)

if(NOT OPTION_NO_LUAMODULES)
	if(TARGET_MACHINE STREQUAL "x86" OR TARGET_MACHINE STREQUAL "x64")
		add_subdirectory(test012)
	endif()
endif()

add_subdirectory(test013)

if(NOT OPTION_NO_LUAMODULES)
	add_subdirectory(test014)
endif()

add_subdirectory(test015)
