cmake_minimum_required(VERSION 3.3.0)

configure_file(sdmdirscfg.h.in "${CMAKE_CURRENT_BINARY_DIR}/sdmdirscfg.h")
configure_file(version.tex.in "${PROJECT_SOURCE_DIR}/doc/manual/version.tex")

# Configure version file, including Git commit hash (if available)

configure_file(GitCommitHash.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/GitCommitHash.cmake" @ONLY)
add_custom_target(ConfigureVersionFile
	${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/GitCommitHash.cmake"
	BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/sdmvercfg.h")

# Library

add_library(config STATIC sdmconfig.cpp)

add_dependencies(config ConfigureVersionFile)

target_include_directories(config PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(config PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(config utils u8e)
