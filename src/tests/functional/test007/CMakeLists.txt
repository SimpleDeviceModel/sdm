cmake_minimum_required(VERSION 3.3.0)

set(TESTNAME test007)

file(GLOB_RECURSE LFS_TEST "${PROJECT_SOURCE_DIR}/3rdparty/luafilesystem/*/tests/test.lua")
configure_file(runtest.lua.in "${CMAKE_CURRENT_BINARY_DIR}/runtest.lua")

add_test(NAME ${TESTNAME} COMMAND ${VALGRIND} $<TARGET_FILE:sdmhost> runtest.lua "$<TARGET_FILE_DIR:lfs>")
