cmake_minimum_required(VERSION 3.3.0)

# Export CMake package configuration

file(RELATIVE_PATH INSTALL_PREFIX_FROM_CONFIG_PACKAGE_SLASH
	"${CMAKE_INSTALL_PREFIX}/${CONFIG_PACKAGE_INSTALL_DIR}" "${CMAKE_INSTALL_PREFIX}")

string(REGEX REPLACE "/$" "" INSTALL_PREFIX_FROM_CONFIG_PACKAGE
	"${INSTALL_PREFIX_FROM_CONFIG_PACKAGE_SLASH}")

configure_file(sdm-config.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/sdm-config.cmake" @ONLY)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/sdm-config-version.cmake"
	VERSION "${PRODUCT_VERSION_MAJOR}.${PRODUCT_VERSION_MINOR}.${PRODUCT_VERSION_PATCH}"
	COMPATIBILITY AnyNewerVersion)
	
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/sdm-config.cmake" "${CMAKE_CURRENT_BINARY_DIR}/sdm-config-version.cmake"
	DESTINATION "${CONFIG_PACKAGE_INSTALL_DIR}")

install(EXPORT sdm 
	NAMESPACE sdm::
	FILE sdm-targets.cmake
	DESTINATION "${CONFIG_PACKAGE_INSTALL_DIR}")
