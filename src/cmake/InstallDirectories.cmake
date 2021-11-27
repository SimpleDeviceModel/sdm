# Sets up the installation directory structure
#
# Defines the following variables:
#
# BIN_INSTALL_DIR (executables, DLLs on Windows)
# PLUGINS_INSTALL_DIR (SDM plugins)
# LIB_INSTALL_DIR (static libraries, shared libraries (non-Windows), import libraries (Windows))
# INCLUDE_INSTALL_DIR (headers)
# LUA_MODULES_INSTALL_DIR (Lua native modules)
# LUA_CMODULES_INSTALL_DIR (Lua binary modules)
# QT_INSTALL_DIR (Qt prefix - for Windows)
# DOC_INSTALL_DIR (documentation)
# I18N_INSTALL_DIR (translations)
# SCRIPTS_INSTALL_DIR (Lua scripts for sdmconsole)
# DATA_INSTALL_DIR (various platform-independent data for sdmconsole, e.g. register maps)
# CONFIG_PACKAGE_INSTALL_DIR (CMake config package)
# EXAMPLES_INSTALL_DIR (examples)
#
# All these paths should be relative to CMAKE_INSTALL_PREFIX

cmake_minimum_required(VERSION 3.3.0)

message("Install prefix: ${CMAKE_INSTALL_PREFIX}")

if(WIN32)
	set(BIN_INSTALL_DIR .)
	set(PLUGINS_INSTALL_DIR plugins)
	set(LIB_INSTALL_DIR lib)
	set(INCLUDE_INSTALL_DIR include)
	set(LUA_MODULES_INSTALL_DIR lua)
	set(LUA_CMODULES_INSTALL_DIR lua)
	set(QT_INSTALL_DIR qt)
	set(DOC_INSTALL_DIR doc)
	set(I18N_INSTALL_DIR translations)
	set(SCRIPTS_INSTALL_DIR scripts)
	set(DATA_INSTALL_DIR data)
	set(CONFIG_PACKAGE_INSTALL_DIR cmake)
	set(EXAMPLES_INSTALL_DIR examples)
else()
	set(BIN_INSTALL_DIR bin)
	set(PLUGINS_INSTALL_DIR lib/sdm/plugins)
	set(LIB_INSTALL_DIR lib/sdm)
	set(INCLUDE_INSTALL_DIR include/sdm)
	set(LUA_MODULES_INSTALL_DIR lib/sdm/lua)
	set(LUA_CMODULES_INSTALL_DIR lib/sdm/lua)
	set(DOC_INSTALL_DIR share/sdm/doc)
	set(I18N_INSTALL_DIR share/sdm/translations)
	set(SCRIPTS_INSTALL_DIR share/sdm/scripts)
	set(DATA_INSTALL_DIR share/sdm/data)
	set(CONFIG_PACKAGE_INSTALL_DIR share/sdm/cmake)
	set(EXAMPLES_INSTALL_DIR share/sdm/examples)
	
# Set desktop entry directories
	file(RELATIVE_PATH INSTALL_PREFIX_FROM_HOME "$ENV{HOME}" "${CMAKE_INSTALL_PREFIX}")
	string(SUBSTRING "${INSTALL_PREFIX_FROM_HOME}" 0 2 TMP)
	if(TMP STREQUAL "..")
# Install path is a system directory
		if(CMAKE_INSTALL_PREFIX STREQUAL "/usr")
			set(FREEDESKTOP_INSTALL_DIR /usr/share)
		else()
			set(FREEDESKTOP_INSTALL_DIR /usr/local/share)
		endif()
	else()
# Install path is a user directory
		if("$ENV{XDG_DATA_HOME}")
			set(FREEDESKTOP_INSTALL_DIR "$ENV{XDG_DATA_HOME}")
		else()
			set(FREEDESKTOP_INSTALL_DIR "$ENV{HOME}/.local/share")
		endif()
	endif()
	message("Installing desktop entries to ${FREEDESKTOP_INSTALL_DIR}")
endif()
