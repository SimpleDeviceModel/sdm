cmake_minimum_required(VERSION 3.3.0)

add_library(api INTERFACE)

target_include_directories(api INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/sdk/api>)

if(INCLUDE_INSTALL_DIR)
	install(TARGETS api EXPORT sdm)
	install(DIRECTORY include/ DESTINATION "${INCLUDE_INSTALL_DIR}/sdk/api")
endif()
