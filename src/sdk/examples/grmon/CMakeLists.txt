cmake_minimum_required(VERSION 3.3.0)

add_library(grmon MODULE grmon.cpp)

target_link_libraries(grmon pluginprovider uart)

# to omit "lib*" at the beginning of the plugin file name
set_target_properties(grmon PROPERTIES PREFIX "")

# install binary module + supplementary files

install(TARGETS grmon
	LIBRARY DESTINATION "${PLUGINS_INSTALL_DIR}")

# install sources

install(FILES grmon.cpp grmon.h readme.txt
	DESTINATION "${EXAMPLES_INSTALL_DIR}/grmon")
install(FILES CMakeLists.txt.install
	DESTINATION "${EXAMPLES_INSTALL_DIR}/grmon"
	RENAME CMakeLists.txt)
