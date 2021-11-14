cmake_minimum_required(VERSION 3.3.0)

add_subdirectory(dfic)

add_library(luadfic MODULE luadfic.cpp compoundtype.cpp)

# to omit "lib*" at the beginning of the plugin file name
set_target_properties(luadfic PROPERTIES PREFIX "")

target_link_libraries(luadfic luaserver dfic utils)

install(TARGETS luadfic
	RUNTIME DESTINATION "${LUA_CMODULES_INSTALL_DIR}"
	LIBRARY DESTINATION "${LUA_CMODULES_INSTALL_DIR}")

enable_testing()
