cmake_minimum_required(VERSION 3.3.0)

install(DIRECTORY . DESTINATION "${LUA_MODULES_INSTALL_DIR}"
	PATTERN "CMakeLists.txt" EXCLUDE)
