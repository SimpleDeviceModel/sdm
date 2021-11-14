# Disable warnings
# Note: this file should be included only in CMakeLists.txt
# that are responsible for building third-party code

cmake_minimum_required(VERSION 3.3.0)

if(OPTION_DIAGNOSTIC STREQUAL "FULL")
# FULL diagnostic mode: enable warnings, but don't treat them as errors
	if(GCC_CMDLINE_SYNTAX)
		string(REGEX REPLACE "-Werror" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
		string(REGEX REPLACE "-Werror" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	elseif(MSVC)
		string(REGEX REPLACE "/WX" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
		string(REGEX REPLACE "/WX" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	endif()
else()
# Otherwise, disable warnings
	if(GCC_CMDLINE_SYNTAX)
		string(REGEX REPLACE "-W[^ ]*|-pedantic" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
		string(REGEX REPLACE "-W[^ ]*|-pedantic" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
	elseif(MSVC)
		string(REGEX REPLACE "/W[^ ]*" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /w")
		string(REGEX REPLACE "/W[^ ]*" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w")
	endif()
endif()
