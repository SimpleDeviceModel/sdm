# Note: this file is NOT a part of the original Lua distribution

cmake_minimum_required(VERSION 3.3.0)

###########################
# BUILD SYSTEM TWEAKS
###########################

include(DisableWarningsFor3rdPartyCode)

# Define compatibility options as in the LuaBinaries project
# to improve compatibility with third-party addons
# (see http://luabinaries.sourceforge.net/configuration.html)

add_definitions(-DLUA_COMPAT_5_3)

# Ensure that Lua functions are exported

if(WIN32)
	add_definitions(-DLUA_BUILD_AS_DLL)
endif()

# Enable Unix-specific features when building for Unix
# (note: readline is not used due to license incompatibilities)

if(UNIX)
	add_definitions(-DLUA_USE_POSIX -DLUA_USE_DLOPEN)
endif()

###########################
# LIBRARY
###########################

set(LUA_SRC_PATH "lua-5.4.3/src")

set(LUA_SRC_LIST "${LUA_SRC_PATH}/lapi.c" "${LUA_SRC_PATH}/lauxlib.c" "${LUA_SRC_PATH}/lbaselib.c" "${LUA_SRC_PATH}/lcode.c" "${LUA_SRC_PATH}/lcorolib.c" "${LUA_SRC_PATH}/lctype.c" "${LUA_SRC_PATH}/ldblib.c" "${LUA_SRC_PATH}/ldebug.c" "${LUA_SRC_PATH}/ldo.c" "${LUA_SRC_PATH}/ldump.c" "${LUA_SRC_PATH}/lfunc.c" "${LUA_SRC_PATH}/lgc.c" "${LUA_SRC_PATH}/linit.c" "${LUA_SRC_PATH}/liolib.c" "${LUA_SRC_PATH}/llex.c" "${LUA_SRC_PATH}/lmathlib.c" "${LUA_SRC_PATH}/lmem.c" "${LUA_SRC_PATH}/loadlib.c" "${LUA_SRC_PATH}/lobject.c" "${LUA_SRC_PATH}/lopcodes.c" "${LUA_SRC_PATH}/loslib.c" "${LUA_SRC_PATH}/lparser.c" "${LUA_SRC_PATH}/lstate.c" "${LUA_SRC_PATH}/lstring.c" "${LUA_SRC_PATH}/lstrlib.c" "${LUA_SRC_PATH}/ltable.c" "${LUA_SRC_PATH}/ltablib.c" "${LUA_SRC_PATH}/ltm.c" "${LUA_SRC_PATH}/lundump.c" "${LUA_SRC_PATH}/lutf8lib.c" "${LUA_SRC_PATH}/lvm.c" "${LUA_SRC_PATH}/lzio.c")

add_library(lua SHARED ${LUA_SRC_LIST})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_link_libraries(lua dl)
endif()

target_include_directories(lua PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${LUA_SRC_PATH}>
	$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/lua>)

# Ensure that all symbols are exported (override global CMAKE_C_VISIBILITY_PRESET value)

set_target_properties(lua PROPERTIES C_VISIBILITY_PRESET default)

# Set output library name (consistent with LuaBinaries guidelines)

set_target_properties(lua PROPERTIES OUTPUT_NAME lua54)

# On Windows, use "luaXX.dll" file name, without "lib" prefix (even with GCC)
# for compatibility with the LuaBinaries project (http://luabinaries.sourceforge.net/)

if(WIN32)
	set_target_properties(lua PROPERTIES PREFIX "")
endif()

###########################
# INSTALL
###########################

install(TARGETS lua
	EXPORT sdm
	RUNTIME DESTINATION "${BIN_INSTALL_DIR}"
	LIBRARY DESTINATION "${LIB_INSTALL_DIR}"
	ARCHIVE DESTINATION "${LIB_INSTALL_DIR}")

install(FILES
		"${LUA_SRC_PATH}/lua.h"
		"${LUA_SRC_PATH}/luaconf.h"
		"${LUA_SRC_PATH}/lauxlib.h"
		"${LUA_SRC_PATH}/lualib.h"
		"${LUA_SRC_PATH}/lua.hpp"
	DESTINATION "${INCLUDE_INSTALL_DIR}/lua")

install(DIRECTORY "${LUA_SRC_PATH}/../doc/"
	DESTINATION "${DOC_INSTALL_DIR}/lua")
