cmake_minimum_required(VERSION 3.3.0)

###########################
# BUILD SYSTEM TWEAKS
###########################

include(SetUpRpath)

###########################
# TARGET
###########################

# Main executable

add_executable(sdmhost main.cpp textconsole.cpp)

target_link_libraries(sdmhost config sdmplug luaserver luabridge u8e)

# Headless executable for Windows

if(WIN32)
	add_executable(sdmhostw WIN32 main.cpp textconsole.cpp)
	set_target_properties(sdmhostw PROPERTIES COMPILE_DEFINITIONS SDMHOST_NEEDS_WINMAIN)
	target_link_libraries(sdmhostw config sdmplug luaserver luabridge u8e)
endif()

###########################
# INSTALL
###########################

install(TARGETS sdmhost
	EXPORT sdm
	DESTINATION "${BIN_INSTALL_DIR}")

if(WIN32)
	install(TARGETS sdmhostw
		EXPORT sdm
		DESTINATION "${BIN_INSTALL_DIR}")
endif()
