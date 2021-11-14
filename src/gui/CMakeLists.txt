cmake_minimum_required(VERSION 3.3.0)

###########################
# BUILD SYSTEM TWEAKS
###########################

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets)
find_package(Qt5Svg)

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
	install(FILES "$<TARGET_FILE:Qt5::QWindowsIntegrationPlugin>" DESTINATION "${QT_INSTALL_DIR}/platforms")
	install(FILES "$<TARGET_FILE:Qt5::QSvgIconPlugin>" DESTINATION "${QT_INSTALL_DIR}/iconengines")
	install(FILES "$<TARGET_FILE:Qt5::QJpegPlugin>" DESTINATION "${QT_INSTALL_DIR}/imageformats")
	install(FILES "$<TARGET_FILE:Qt5::QTiffPlugin>" DESTINATION "${QT_INSTALL_DIR}/imageformats")
endif()
