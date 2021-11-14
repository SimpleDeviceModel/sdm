cmake_minimum_required(VERSION 3.3.0)

set(TESTNAME u8e_test006)

add_executable(${TESTNAME} test.cpp)

target_link_libraries(${TESTNAME} u8e)

add_test(NAME ${TESTNAME} COMMAND "$<TARGET_FILE:${TESTNAME}>" Hello Привет)

set_tests_properties(${TESTNAME} PROPERTIES PASS_REGULAR_EXPRESSION "Количество аргументов: 3")
