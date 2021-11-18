cmake_minimum_required(VERSION 3.3.0)

add_library(uartdemo MODULE uartdemo.cpp)

target_link_libraries(uartdemo pluginprovider uart)

# to omit "lib*" at the beginning of the plugin file name
set_target_properties(uartdemo PROPERTIES PREFIX "")

# install binary module + supplementary files

install(TARGETS uartdemo
	LIBRARY DESTINATION "${PLUGINS_INSTALL_DIR}")
install(DIRECTORY data/ DESTINATION "${DATA_INSTALL_DIR}/uartdemo")

# install sources

install(FILES uartdemo.cpp uartdemo.h readme.txt
	DESTINATION "${EXAMPLES_INSTALL_DIR}/uartdemo")
install(FILES CMakeLists.txt.install
	DESTINATION "${EXAMPLES_INSTALL_DIR}/uartdemo"
	RENAME CMakeLists.txt)
install(DIRECTORY data DESTINATION "${EXAMPLES_INSTALL_DIR}/uartdemo")
install(DIRECTORY sketch DESTINATION "${EXAMPLES_INSTALL_DIR}/uartdemo")
