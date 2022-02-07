cmake_minimum_required(VERSION 3.3.0)

add_library(uart INTERFACE)

target_sources(uart INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/uart.cpp>
	$<INSTALL_INTERFACE:${LIB_INSTALL_DIR}/sdk/uart/uart.cpp>)

if(WIN32)
	target_sources(uart INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/win32/uartimpl.cpp>
		$<INSTALL_INTERFACE:${LIB_INSTALL_DIR}/sdk/uart/win32/uartimpl.cpp>)
else()
	target_sources(uart INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/posix/uartimpl.cpp>
		$<INSTALL_INTERFACE:${LIB_INSTALL_DIR}/sdk/uart/posix/uartimpl.cpp>)
endif()

target_include_directories(uart INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/sdk/uart>)

###########################
# INSTALL
###########################

install(TARGETS uart EXPORT sdm)

install(DIRECTORY include/
	DESTINATION "${INCLUDE_INSTALL_DIR}/sdk/uart")

install(DIRECTORY src/
	DESTINATION "${LIB_INSTALL_DIR}/sdk/uart")
