cmake_minimum_required(VERSION 3.3.0)

add_library(ipsockets src/ipsocket.cpp)

target_include_directories(ipsockets PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/sdk/ipsockets>)

if(WIN32)
	target_link_libraries(ipsockets ws2_32 iphlpapi)
endif()

set_target_properties(ipsockets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

###########################
# INSTALL
###########################

install(TARGETS ipsockets
	EXPORT sdm
	LIBRARY DESTINATION "${LIB_INSTALL_DIR}/sdk"
	ARCHIVE DESTINATION "${LIB_INSTALL_DIR}/sdk")

install(DIRECTORY include/
	DESTINATION "${INCLUDE_INSTALL_DIR}/sdk/ipsockets")
