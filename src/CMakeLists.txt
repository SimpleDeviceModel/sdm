cmake_minimum_required(VERSION 3.3.0)
project(SDM)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

###########################
# BUILD OPTIONS
###########################

option(OPTION_DIAGNOSTIC "Employ additional diagnostics")
option(OPTION_LUA_SYSTEM "Link against Lua library installed on this system")
option(OPTION_NO_LUAMODULES "Don't build optional Lua modules")
option(OPTION_NO_QT "Don't use Qt")

# By default, use app-local deployment of the MSVC runtime for MSVC 2013,
# central deployment (vcredist_*.exe) for later versions

if(MSVC)
	if(MSVC_VERSION GREATER 1800)
		option(OPTION_USE_VCREDIST "Use central deployment for Microsoft Visual C++ Runtime" ON)
	else()
		option(OPTION_USE_VCREDIST "Use central deployment for Microsoft Visual C++ Runtime" OFF)
	endif()
endif()

if(UNIX)
	option(OPTION_VALGRIND "Enable Valgrind in the main test suite")
	option(OPTION_NO_ICONS "Skip installation of desktop entries")
	option(OPTION_NO_LINENOISE "Don't use linenoise for sdmhost")
endif()

include(Version)
include(GlobalBuildConfig)
include(InstallDirectories)
include(DiagnosticMode)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

###########################
# LIBRARIES
###########################

# utils
add_subdirectory(utils)

# config
add_subdirectory(config)

# SDK
add_subdirectory(sdk)

# u8e
add_subdirectory(u8e)

# sdmplug
add_subdirectory(sdmplug)

# lua
if(NOT (PARAMETER_LUA_LINKMODE STREQUAL "SYSTEM"))
	add_subdirectory(3rdparty/lua)
endif()

# luaserver
add_subdirectory(luaserver)

# luabridge
add_subdirectory(luabridge)

# Optional Lua modules
if(NOT OPTION_NO_LUAMODULES)
	add_subdirectory(luamodules)
endif()

if(UNIX AND NOT OPTION_NO_LINENOISE)
	add_subdirectory(3rdparty/linenoise)
endif()

###########################
# EXECUTABLES
###########################

add_subdirectory(sdmhost)

if(NOT OPTION_NO_QT)
	add_subdirectory(gui)
endif()

###########################
# TESTS
###########################

enable_testing()

add_subdirectory(tests/functional)

###########################
# INSTALL DOCUMENTATION
###########################

add_subdirectory(doc)

###########################
# CREATE CMAKE PACKAGE
###########################

add_subdirectory(cmake/package)

###########################
# CREATE INSTALLER
###########################

add_subdirectory(cmake/installer)

###########################
# DEPLOYMENT
###########################

if(WIN32)
	add_subdirectory(cmake/dlldeptool)
endif()
