cmake_minimum_required(VERSION 3.3.0)

set(TESTNAME test010)

configure_file(runtest.lua.in "${CMAKE_CURRENT_BINARY_DIR}/runtest.lua")

add_test(NAME ${TESTNAME} COMMAND ${VALGRIND} $<TARGET_FILE:sdmhost> runtest.lua "$<TARGET_FILE:testplugin>")
