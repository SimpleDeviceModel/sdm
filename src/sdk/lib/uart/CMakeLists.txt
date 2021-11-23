cmake_minimum_required(VERSION 3.3.0)

set(UART_SRC src/uart.cpp)

if(WIN32)
	list(APPEND UART_SRC src/win32/uartimpl.cpp)
else()
	list(APPEND UART_SRC src/posix/uartimpl.cpp)
endif()

add_library(uart ${UART_SRC})

target_include_directories(uart PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/sdk/uart>)

target_include_directories(uart PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

set_target_properties(uart PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

###########################
# INSTALL
###########################

install(TARGETS uart
	EXPORT sdm
	LIBRARY DESTINATION "${LIB_INSTALL_DIR}/sdk"
	ARCHIVE DESTINATION "${LIB_INSTALL_DIR}/sdk")

install(DIRECTORY include/
	DESTINATION "${INCLUDE_INSTALL_DIR}/sdk/uart")
