cmake_minimum_required(VERSION 3.3.0)

add_executable(dlldeptool main.cpp winpath.cpp peimports.cpp)

target_link_libraries(dlldeptool u8e)

# Find Qt DLL directory

if(NOT OPTION_NO_QT)
	find_package(Qt5Core)
	get_target_property(QtCore_location Qt5::Core LOCATION)
	string(REGEX REPLACE "/[^/]+$" "" QTLIBPATH ${QtCore_location})
else()
	set(QTLIBPATH FALSE)
endif()

# Determine whether we need to deploy MSVC runtime locally

if(MSVC AND OPTION_USE_VCREDIST)
	set(NOVCRUNTIME "--no-vc-runtime")
endif()

# Generate script to run dlldeptool at install time

configure_file(RunDepTool.cmake.in RunDepTool.cmake @ONLY)

# Run post-install script to collect dependencies

install(TARGETS dlldeptool DESTINATION "${BIN_INSTALL_DIR}")
install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/RunDepTool.cmake")
