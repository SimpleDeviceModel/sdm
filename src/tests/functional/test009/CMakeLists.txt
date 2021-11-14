cmake_minimum_required(VERSION 3.3.0)

set(TESTNAME test009)

include_directories(../common)

add_executable(${TESTNAME} testmain.cpp)

target_link_libraries(${TESTNAME} luaserver)

configure_file(runtest1.lua "${CMAKE_CURRENT_BINARY_DIR}/runtest1.lua" COPYONLY)
configure_file(runtest2.lua "${CMAKE_CURRENT_BINARY_DIR}/runtest2.lua" COPYONLY)

# Don't use Valgrind here since the test is already too slow

add_test(NAME ${TESTNAME} COMMAND "$<TARGET_FILE:${TESTNAME}>")
