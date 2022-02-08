cmake_minimum_required(VERSION 3.3.0)

add_library(ipsockets INTERFACE)

target_sources(ipsockets INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/ipsocket.cpp>
	$<INSTALL_INTERFACE:${LIB_INSTALL_DIR}/sdk/ipsockets/ipsocket.cpp>)

target_include_directories(ipsockets INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/sdk/ipsockets>)

if(WIN32)
	target_link_libraries(ipsockets INTERFACE ws2_32 iphlpapi)
endif()

###########################
# INSTALL
###########################

install(TARGETS ipsockets EXPORT sdm)

install(DIRECTORY include/
	DESTINATION "${INCLUDE_INSTALL_DIR}/sdk/ipsockets")

install(DIRECTORY src/
	DESTINATION "${LIB_INSTALL_DIR}/sdk/ipsockets")
