######################################
# Global build system configuration
######################################

cmake_minimum_required(VERSION 3.3.0)

# Check whether we are using a GCC-compatible compiler

if(CMAKE_C_COMPILER_ID STREQUAL GNU)
	set(GCC_CMDLINE_SYNTAX TRUE)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL Clang)
	set(GCC_CMDLINE_SYNTAX TRUE)
endif()

# Sets up default build type if it wasn't set by the user (only for single-configuration generators)

if(NOT CMAKE_CONFIGURATION_TYPES)
	if(NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE Release)
		message("Build type: ${CMAKE_BUILD_TYPE} (set CMAKE_BUILD_TYPE to change)")
	else()
		message("Build type: ${CMAKE_BUILD_TYPE}")
	endif()
endif()

# Check for Lua installation

if(OPTION_LUA_SYSTEM)
	find_package(Lua)
	
	if(NOT LUA_FOUND)
		message(FATAL_ERROR "Linking against system-installed Lua library was requested, but it can't be found")
	elseif(LUA_VERSION_STRING VERSION_LESS "5.3")
		message(FATAL_ERROR "Lua ${LUA_VERSION_STRING} found on this system, but SDM requires at least 5.3. Consider using bundled version instead.")
	endif()
	
	set(PARAMETER_LUA_LINKMODE "SYSTEM")
	message("Using Lua ${LUA_VERSION_STRING} found on this system")
	add_library(lua INTERFACE)
	target_include_directories(lua INTERFACE "${LUA_INCLUDE_DIR}")
	target_link_libraries(lua INTERFACE "${LUA_LIBRARIES}")
	install(TARGETS lua EXPORT sdm)
	add_definitions(-DUSING_SYSTEM_LUA)
else()
	set(PARAMETER_LUA_LINKMODE "SHARED")
	message("Using Lua included in the ${CMAKE_PROJECT_NAME} distribution")
endif()

# Enable C++11 globally

set(CMAKE_CXX_STANDARD 11) # enable C++11

# Check for optional compiler features

message("Optional compiler features:")

list(FIND CMAKE_CXX_COMPILE_FEATURES cxx_noexcept NOEXCEPT_PRESENT)
list(FIND CMAKE_CXX_COMPILE_FEATURES cxx_reference_qualified_functions REF_QUALIFIERS_PRESENT)

if(NOT NOEXCEPT_PRESENT EQUAL -1)
	message("	noexcept: present")
	add_definitions(-DHAVE_NOEXCEPT)
else()
	message("	noexcept: not present")
endif()

if(NOT REF_QUALIFIERS_PRESENT EQUAL -1)
	message("	reference qualifiers: present")
	add_definitions(-DHAVE_REF_QUALIFIERS)
else()
	message("	reference qualifiers: not present")
endif()

# Set up warning level

if(GCC_CMDLINE_SYNTAX)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -Wall -Wextra -Wno-unused-parameter")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wextra -Wno-unused-parameter")
endif()

# Don't export unnecessary symbols from the executables

if(GCC_CMDLINE_SYNTAX)
	string(REGEX REPLACE "-rdynamic" "" CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS}")
	string(REGEX REPLACE "-rdynamic" "" CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS}")
endif()

# Hide symbols by default when using GCC. In order to explicitly make
# symbol public, use __attribute__((__visibility__("default")))
# (for GCC version >= 4). To make all symbols public, set C_VISIBILITY_PRESET
# and/or CXX_VISIBILITY_PRESET target properties.

if(GCC_CMDLINE_SYNTAX)
	set(CMAKE_C_VISIBILITY_PRESET hidden)
	set(CMAKE_CXX_VISIBILITY_PRESET hidden)
endif()
