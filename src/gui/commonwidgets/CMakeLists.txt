cmake_minimum_required(VERSION 3.3.0)

###########################
# LIBRARY
###########################

add_library(commonwidgets STATIC autoresizingtable.cpp codeeditor.cpp consolewidget.cpp dockwrapper.cpp filedialogex.cpp fileselector.cpp fontutils.cpp formdialog.cpp iconbutton.cpp luahighlighter.cpp optionselectordialog.cpp plotterabstractscene.cpp plotteraddcursordialog.cpp plotterbarscene.cpp plotterbinaryscene.cpp plotterbitmapscene.cpp plottervideoscene.cpp plottercursorwidget.cpp plotterlayersdialog.cpp plotterscrollarea.cpp plotterwidget.cpp statusprogresswidget.cpp textviewer.cpp resources/commonwidgets.qrc)

target_include_directories(commonwidgets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(translations)

target_link_libraries(commonwidgets Qt5::Widgets Qt5::Svg utils)

# Ensure that whatever is linked to this library uses the same optional compiler features

if(NOT NOEXCEPT_PRESENT EQUAL -1)
	target_compile_definitions(commonwidgets INTERFACE -DHAVE_NOEXCEPT)
endif()

if(NOT REF_QUALIFIERS_PRESENT EQUAL -1)
	target_compile_definitions(commonwidgets INTERFACE -DHAVE_REF_QUALIFIERS)
endif()
