cmake_minimum_required(VERSION 3.3.0)

set(TESTNAME test014)

configure_file(runtest.lua "${CMAKE_CURRENT_BINARY_DIR}/runtest.lua" COPYONLY)

add_test(NAME ${TESTNAME} COMMAND ${VALGRIND} $<TARGET_FILE:sdmhost> runtest.lua "$<TARGET_FILE_DIR:luaipsockets>")
