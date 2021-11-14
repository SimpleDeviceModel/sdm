cmake_minimum_required(VERSION 3.3.0)
project(DFIC)

# Configure the build environment

if(CMAKE_C_COMPILER_ID STREQUAL GNU OR CMAKE_C_COMPILER_ID STREQUAL Clang)
	set(GCC_CMDLINE_SYNTAX TRUE)
endif()

if(GCC_CMDLINE_SYNTAX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")
endif()

###########################
# LIBRARY
###########################

add_library(dfic dfic.cpp)

target_include_directories(dfic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(dfic PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

###########################
# TESTS
###########################

enable_testing()

add_subdirectory(tests)
