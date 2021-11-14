cmake_minimum_required(VERSION 3.3.0)

add_library(simpleplugin MODULE simpleplugin.c)

target_link_libraries(simpleplugin api)

# to omit "lib*" at the beginning of the plugin file name
set_target_properties(simpleplugin PROPERTIES PREFIX "")

install(FILES simpleplugin.c
	DESTINATION "${EXAMPLES_INSTALL_DIR}/simpleplugin")
install(FILES CMakeLists.txt.install
	DESTINATION "${EXAMPLES_INSTALL_DIR}/simpleplugin"
	RENAME CMakeLists.txt)
