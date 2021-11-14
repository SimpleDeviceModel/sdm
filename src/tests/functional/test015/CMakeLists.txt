cmake_minimum_required(VERSION 3.3.0)

set(TESTNAME test015)
set(TESTMODNAME ${TESTNAME}_mod)
set(TESTLIBNAME ${TESTNAME}_lib)
set(TESTHOSTNAME ${TESTNAME}_host)

# Test shared library

add_library(${TESTLIBNAME} SHARED testlib.cpp)

# Test module

add_library(${TESTMODNAME} MODULE testmod.cpp)
target_link_libraries(${TESTMODNAME} ${TESTLIBNAME})
set_target_properties(${TESTMODNAME} PROPERTIES PREFIX "")

# Test executable

add_executable(${TESTHOSTNAME} testhost.cpp)
target_link_libraries(${TESTHOSTNAME} utils ${TESTLIBNAME})

add_test(NAME ${TESTNAME} COMMAND "$<TARGET_FILE:${TESTHOSTNAME}>" "$<TARGET_FILE:${TESTMODNAME}>")
