# Handles OPTION_DIAGNOSTIC which turns additional diagnostics on:
#
# * treat warnings as errors
# * link heap checker to sdmhost
# * use safe (bounds-checked) STL iterators if possible
# * allow assert() even for Release builds

cmake_minimum_required(VERSION 3.3.0)

if(OPTION_DIAGNOSTIC STREQUAL "FULL")
	message("Diagnostic mode: FULL")
elseif(OPTION_DIAGNOSTIC)
	message("Diagnostic mode: ON")
else()
	message("Diagnostic mode: OFF")
endif()

# Treat warning as errors

if(OPTION_DIAGNOSTIC)
	if(GCC_CMDLINE_SYNTAX)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
	elseif(MSVC)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
	endif()
endif()

# Activate checked STL iterators for MSVC and GCC-compatible toolchains
# Checked iterators are enabled under MSVC for Release builds only when GUI is not build
# because of ABI incompatibility with compiled Qt (but they are automatically activated
# for debug builds)

if(OPTION_DIAGNOSTIC)
	if(GCC_CMDLINE_SYNTAX)
		add_definitions(-D_GLIBCXX_DEBUG)
	elseif(MSVC AND OPTION_NO_QT)
		add_definitions(-D_SECURE_SCL)
	endif()
endif()

# Allow assert() for Release builds by suppressing -DNDEBUG

if(OPTION_DIAGNOSTIC)
	string(REGEX REPLACE ".DNDEBUG" "" CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
	string(REGEX REPLACE ".DNDEBUG" "" CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
	string(REGEX REPLACE ".DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
	string(REGEX REPLACE ".DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
endif()
