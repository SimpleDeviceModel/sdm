cmake_minimum_required(VERSION 3.3.0)

add_subdirectory(simpleplugin)
add_subdirectory(testplugin)
add_subdirectory(uartdemo)

install(FILES readme.txt
	DESTINATION "${EXAMPLES_INSTALL_DIR}")
