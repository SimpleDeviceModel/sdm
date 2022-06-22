# Note: this file is NOT a part of the original Linenoise distribution

cmake_minimum_required(VERSION 3.3.0)

include(DisableWarningsFor3rdPartyCode)

add_library(linenoise STATIC linenoise.cpp ConvertUTF.cpp wcwidth.cpp)

target_include_directories(linenoise PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
