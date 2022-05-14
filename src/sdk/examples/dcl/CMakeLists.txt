cmake_minimum_required(VERSION 3.3.0)

add_library(dcl MODULE dcl.cpp)

target_link_libraries(dcl pluginprovider uart)

# to omit "lib*" at the beginning of the plugin file name
set_target_properties(dcl PROPERTIES PREFIX "")

# install binary module + supplementary files

install(TARGETS dcl
	LIBRARY DESTINATION "${PLUGINS_INSTALL_DIR}")

# install sources

install(FILES dcl.cpp dcl.h readme.txt
	DESTINATION "${EXAMPLES_INSTALL_DIR}/dcl")
install(FILES CMakeLists.txt.install
	DESTINATION "${EXAMPLES_INSTALL_DIR}/dcl"
	RENAME CMakeLists.txt)
