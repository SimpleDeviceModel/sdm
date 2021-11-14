cmake_minimum_required(VERSION 3.3.0)

set(TESTNAME test008)

add_executable(${TESTNAME} testmain.cpp)

target_link_libraries(${TESTNAME} utils)

add_test(NAME ${TESTNAME} COMMAND ${VALGRIND} "$<TARGET_FILE:${TESTNAME}>")
