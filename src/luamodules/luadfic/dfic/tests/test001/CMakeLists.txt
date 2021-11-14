cmake_minimum_required(VERSION 3.3.0)

set(TESTNAME dfic_test001)
set(TESTLIBNAME ${TESTNAME}_lib)
set(TESTHOSTNAME ${TESTNAME}_host)

# Test library

add_library(${TESTLIBNAME} MODULE testlib.cpp)

target_include_directories(${TESTLIBNAME} PRIVATE "${PROJECT_SOURCE_DIR}")
set_target_properties(${TESTLIBNAME} PROPERTIES PREFIX "")

# Test executable

add_executable(${TESTHOSTNAME} testhost.cpp)

target_link_libraries(${TESTHOSTNAME} dfic utils)

add_test(NAME ${TESTNAME} COMMAND "$<TARGET_FILE:${TESTHOSTNAME}>" "$<TARGET_FILE:${TESTLIBNAME}>")
