cmake_minimum_required(VERSION 3.3.0)

add_library(luaipsockets MODULE luaipsockets.cpp)

# to omit "lib*" at the beginning of the plugin file name
set_target_properties(luaipsockets PROPERTIES PREFIX "")

target_link_libraries(luaipsockets luaserver ipsockets)

install(TARGETS luaipsockets
	RUNTIME DESTINATION "${LUA_CMODULES_INSTALL_DIR}"
	LIBRARY DESTINATION "${LUA_CMODULES_INSTALL_DIR}")
