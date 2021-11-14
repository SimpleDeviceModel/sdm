cmake_minimum_required(VERSION 3.3.0)

set(TESTNAME u8e_test001)

add_executable(${TESTNAME} test.cpp)

target_link_libraries(${TESTNAME} u8e)

configure_file("test.utf8.orig" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
configure_file("test.cp1251.expected" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)

add_test(NAME ${TESTNAME} COMMAND "$<TARGET_FILE:${TESTNAME}>")

add_test(NAME ${TESTNAME}_compare_1 COMMAND ${CMAKE_COMMAND} -E compare_files test.cp1251 test.cp1251.expected)
add_test(NAME ${TESTNAME}_compare_2 COMMAND ${CMAKE_COMMAND} -E compare_files test.utf8 test.utf8.orig)
