cmake_minimum_required(VERSION 3.3.0)

add_library(pluginprovider INTERFACE)

target_sources(pluginprovider INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/sdmprovider.cpp>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/sdmproperty.cpp>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/sdmexport.cpp>
	$<INSTALL_INTERFACE:${LIB_INSTALL_DIR}/sdk/pluginprovider/sdmprovider.cpp>
	$<INSTALL_INTERFACE:${LIB_INSTALL_DIR}/sdk/pluginprovider/sdmproperty.cpp>
	$<INSTALL_INTERFACE:${LIB_INSTALL_DIR}/sdk/pluginprovider/sdmexport.cpp>)

target_include_directories(pluginprovider INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}/sdk/pluginprovider>)

target_link_libraries(pluginprovider INTERFACE api)

###########################
# INSTALL
###########################

install(TARGETS pluginprovider EXPORT sdm)

install(DIRECTORY include/
	DESTINATION "${INCLUDE_INSTALL_DIR}/sdk/pluginprovider")

install(DIRECTORY src/
	DESTINATION "${LIB_INSTALL_DIR}/sdk/pluginprovider")
