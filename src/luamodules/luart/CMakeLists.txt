cmake_minimum_required(VERSION 3.3.0)

add_library(luart MODULE luart.cpp)

# to omit "lib*" at the beginning of the plugin file name
set_target_properties(luart PROPERTIES PREFIX "")

target_link_libraries(luart luaserver uart)

install(TARGETS luart
	RUNTIME DESTINATION "${LUA_CMODULES_INSTALL_DIR}"
	LIBRARY DESTINATION "${LUA_CMODULES_INSTALL_DIR}")
