cmake_minimum_required(VERSION 3.3.0)

install(DIRECTORY . DESTINATION "${SCRIPTS_INSTALL_DIR}"
	PATTERN "CMakeLists.txt" EXCLUDE)
