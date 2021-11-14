cmake_minimum_required(VERSION 3.3.0)

set(TESTNAME test002)

include_directories(../common)

add_library(${TESTNAME} MODULE testmain.cpp ../common/testcommon.cpp)

# to omit "lib*" at the beginning of the plugin file name
set_target_properties(${TESTNAME} PROPERTIES PREFIX "" OUTPUT_NAME testlib)

target_link_libraries(${TESTNAME} luaserver)

configure_file(runtest.lua.in "${CMAKE_CURRENT_BINARY_DIR}/runtest.lua")

add_test(NAME ${TESTNAME} COMMAND ${VALGRIND} $<TARGET_FILE:sdmhost> runtest.lua)
