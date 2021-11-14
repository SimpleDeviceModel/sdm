cmake_minimum_required(VERSION 3.3.0)

set(TESTNAME u8e_test004)

add_executable(${TESTNAME} test.cpp)

target_link_libraries(${TESTNAME} u8e)

# Don't run it
#add_test(NAME ${TESTNAME} COMMAND "$<TARGET_FILE:${TESTNAME}>")
