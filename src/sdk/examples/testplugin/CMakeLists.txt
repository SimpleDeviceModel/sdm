cmake_minimum_required(VERSION 3.3.0)

add_library(testplugin MODULE testplugin.cpp videoframe.cpp)

target_link_libraries(testplugin pluginprovider)

# to omit "lib*" at the beginning of the plugin file name
set_target_properties(testplugin PROPERTIES PREFIX "")

# install binary module + supplementary files

install(TARGETS testplugin
	LIBRARY DESTINATION "${PLUGINS_INSTALL_DIR}")
install(DIRECTORY data/ DESTINATION "${DATA_INSTALL_DIR}/testplugin")

# install sources

install(FILES testplugin.cpp testplugin.h videoframe.cpp videoframe.h
	DESTINATION "${EXAMPLES_INSTALL_DIR}/testplugin")
install(FILES CMakeLists.txt.install
	DESTINATION "${EXAMPLES_INSTALL_DIR}/testplugin"
	RENAME CMakeLists.txt)
install(DIRECTORY data DESTINATION "${EXAMPLES_INSTALL_DIR}/testplugin")
