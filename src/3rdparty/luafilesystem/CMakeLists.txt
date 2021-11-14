# Note: this file is NOT a part of the original LuaFileSystem distribution

cmake_minimum_required(VERSION 3.3.0)

include(DisableWarningsFor3rdPartyCode)

###########################
# LIBRARY
###########################

set(LFS_SRC_PATH "luafilesystem-1_8_0/src")

add_library(lfs MODULE "${LFS_SRC_PATH}/lfs.c")

# Ensure that all symbols are exported (override global CMAKE_C_VISIBILITY_PRESET value)

set_target_properties(lfs PROPERTIES C_VISIBILITY_PRESET default)

# Lua expects package file name without "lib" prefix

set_target_properties(lfs PROPERTIES PREFIX "")

target_link_libraries(lfs lua)

###########################
# INSTALL
###########################

install(TARGETS lfs
	RUNTIME DESTINATION "${LUA_CMODULES_INSTALL_DIR}"
	LIBRARY DESTINATION "${LUA_CMODULES_INSTALL_DIR}")

install(DIRECTORY "${LFS_SRC_PATH}/../doc/us/"
	DESTINATION "${DOC_INSTALL_DIR}/luafilesystem")
