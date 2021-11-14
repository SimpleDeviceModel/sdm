cmake_minimum_required(VERSION 3.3.0)

set(TESTNAME test011)

add_executable(${TESTNAME} testmain.cpp)

target_link_libraries(${TESTNAME} utils)

if(UNIX)
	target_link_libraries(${TESTNAME} pthread)
endif()

add_test(NAME ${TESTNAME} COMMAND ${VALGRIND} "$<TARGET_FILE:${TESTNAME}>")
