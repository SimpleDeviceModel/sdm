cmake_minimum_required(VERSION 3.3.0)

set(TESTNAME test012)

add_library(${TESTNAME} MODULE testlib.cpp)

# to omit "lib*" at the beginning of the plugin file name
set_target_properties(${TESTNAME} PROPERTIES PREFIX "" OUTPUT_NAME testlib)

configure_file(runtest.lua "${CMAKE_CURRENT_BINARY_DIR}/runtest.lua" COPYONLY)

add_test(NAME ${TESTNAME} COMMAND ${VALGRIND} $<TARGET_FILE:sdmhost> runtest.lua "$<TARGET_FILE:${TESTNAME}>" "$<TARGET_FILE:testplugin>" "$<TARGET_FILE_DIR:luadfic>")
set_tests_properties(${TESTNAME} PROPERTIES DEPENDS ${TESTNAME}_prepare)
