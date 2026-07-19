cmake_minimum_required(VERSION 3.22)

install(DIRECTORY . DESTINATION "${LUA_MODULES_INSTALL_DIR}"
	PATTERN "CMakeLists.txt" EXCLUDE)
