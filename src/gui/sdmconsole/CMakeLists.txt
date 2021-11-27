cmake_minimum_required(VERSION 3.3.0)

include(SetUpRpath)

###########################
# EXECUTABLE
###########################

add_executable(sdmconsole WIN32 main.cpp cmdargs.cpp extrakeywords.cpp appwidelock.cpp hints.cpp mainwindow.cpp luagui.cpp luadialogserver.cpp luaformdialog.cpp luaplotterwidget.cpp luatextviewer.cpp docroot.cpp docpanels.cpp propertyeditor.cpp sidebar.cpp luawidget.cpp registermapwidget.cpp registermapengine.cpp registermapworker.cpp registermaptable.cpp registermapitem.cpp registerconfigdialog.cpp registermapxml.cpp openplugindialog.cpp streamreader.cpp streamselector.cpp mhdbwriter.cpp luaserverqt.cpp translations.cpp resources/sdmconsole.qrc win32/sdmconsole.rc)

add_definitions(-DQT_NO_DEPRECATED_WARNINGS)

add_subdirectory(translations)
add_subdirectory(scripts)

target_link_libraries(sdmconsole Qt5::Widgets config commonwidgets luaserver sdmplug luabridge utils u8e api)

if(UNIX)
	target_link_libraries(sdmconsole pthread)
	configure_file(freedesktop/sdmconsole.desktop.in sdmconsole.desktop @ONLY)
endif()

###########################
# INSTALL
###########################

install(TARGETS sdmconsole
	EXPORT sdm
	DESTINATION "${BIN_INSTALL_DIR}")

if(UNIX)
	install(FILES "${CMAKE_CURRENT_BINARY_DIR}/sdmconsole.desktop" DESTINATION "${FREEDESKTOP_INSTALL_DIR}/applications")
	INSTALL(FILES resources/icons/appicon.svg DESTINATION "${FREEDESKTOP_INSTALL_DIR}/icons/hicolor/scalable/apps" RENAME sdmconsole.svg)
	INSTALL(FILES freedesktop/sdmconsole48.png DESTINATION "${FREEDESKTOP_INSTALL_DIR}/icons/hicolor/48x48/apps" RENAME sdmconsole.png)
	INSTALL(FILES freedesktop/sdmconsole256.png DESTINATION "${FREEDESKTOP_INSTALL_DIR}/icons/hicolor/256x256/apps" RENAME sdmconsole.png)
endif()
