cmake_minimum_required(VERSION 3.22)

install(DIRECTORY . DESTINATION "${SCRIPTS_INSTALL_DIR}"
	PATTERN "CMakeLists.txt" EXCLUDE)
