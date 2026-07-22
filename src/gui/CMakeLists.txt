cmake_minimum_required(VERSION 3.22)

###########################
# BUILD SYSTEM TWEAKS
###########################

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Core Widgets Svg)

###########################
# TARGETS
###########################

add_subdirectory(commonwidgets)
add_subdirectory(sdmconsole)

###########################
# INSTALL
###########################

# Deploy the essential Qt plugins on Windows

if(WIN32)
	install(FILES "$<TARGET_FILE:Qt6::QWindowsIntegrationPlugin>" DESTINATION "${QT_INSTALL_DIR}/platforms")
	install(FILES "$<TARGET_FILE:Qt6::QSvgIconPlugin>" DESTINATION "${QT_INSTALL_DIR}/iconengines")
	install(FILES "$<TARGET_FILE:Qt6::QJpegPlugin>" DESTINATION "${QT_INSTALL_DIR}/imageformats")
	install(FILES "$<TARGET_FILE:Qt6::QTiffPlugin>" DESTINATION "${QT_INSTALL_DIR}/imageformats")
	install(FILES "$<TARGET_FILE:Qt6::QModernWindowsStylePlugin>" DESTINATION "${QT_INSTALL_DIR}/styles")
endif()
