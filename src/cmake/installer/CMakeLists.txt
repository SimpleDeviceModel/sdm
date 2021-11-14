cmake_minimum_required(VERSION 3.3.0)

###########################
# CREATE NSIS PACKAGE
###########################

if(WIN32 AND (TARGET_MACHINE STREQUAL "x86" OR TARGET_MACHINE STREQUAL "x64"))
	# Set common packaging variables
	
	if(TARGET_MACHINE STREQUAL "x64")
		set(PROGRAM_FILES_ARCH "$PROGRAMFILES64")
	else()
		set(PROGRAM_FILES_ARCH "$PROGRAMFILES")
	endif()
	
	if(DEFINED EXTRA_PLUGINS_DIR)
		set(INSTALL_FLAVOR "-extra")
	endif()
	
	# Find vcredist_*.exe if requested
	
	if(MSVC AND OPTION_USE_VCREDIST)
		set(USE_VCREDIST "USE")
	# Obtain vcredist_*.exe location
		if(NOT VCREDIST_PREFIX)
			string(REGEX MATCH ".*/VC/" VC_PATH "${CMAKE_C_COMPILER}")
			set(VCREDIST_PREFIX "${VC_PATH}redist")
		endif()
		file(GLOB_RECURSE VCREDIST_PATHS "${VCREDIST_PREFIX}/vcredist_${TARGET_MACHINE}.exe" "${VCREDIST_PREFIX}/VC_redist.${TARGET_MACHINE}.exe")
		if(NOT VCREDIST_PATHS)
			message(FATAL_ERROR "Cannot find Microsoft Visual C++ Redistributable package")
		endif()
# Sort list alphabetically and extract the last one, just in case there are multiple versions
		list(SORT VCREDIST_PATHS)
		list(GET VCREDIST_PATHS -1 VCREDIST_PATH)
		string(REPLACE "/" "\\" VCREDIST_PATH_NATIVE "${VCREDIST_PATH}")
	else()
		set(USE_VCREDIST "DONTUSE")
	endif()
	
	# Configure NSIS script
	
	configure_file(make_installer.nsi.in make_installer.nsi @ONLY)
	
	INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/make_installer.nsi" DESTINATION .)
endif()

###########################
# CREATE UNINSTALL SCRIPT
###########################

if(NOT WIN32)
# Remove empty directories unless install prefix is /, /usr or /usr/local
	if(CMAKE_INSTALL_PREFIX STREQUAL ""
		OR CMAKE_INSTALL_PREFIX STREQUAL "/"
		OR CMAKE_INSTALL_PREFIX STREQUAL "/usr"
		OR CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
		
		set(DONTREMOVE "#")
	else()
		set(DONTREMOVE "")
	endif()
	
	configure_file(uninstall.sh.in "${CMAKE_CURRENT_BINARY_DIR}/uninstall.sh")
	file(COPY "${CMAKE_CURRENT_BINARY_DIR}/uninstall.sh"
		DESTINATION "${PROJECT_BINARY_DIR}"
		FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ WORLD_READ)
endif()
