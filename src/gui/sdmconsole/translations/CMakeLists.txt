cmake_minimum_required(VERSION 3.3.0)

find_package(Qt5LinguistTools)

set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM TRUE)

# Obtain Qt base translation path

get_target_property(QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
execute_process(COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_TRANSLATIONS OUTPUT_VARIABLE QMAKE_OUTPUT)
string(STRIP ${QMAKE_OUTPUT} QT_INSTALL_TRANSLATIONS)
set(QTBASE_RU "${QT_INSTALL_TRANSLATIONS}/qtbase_ru.qm")

# Compile sdmconsole translation

qt5_create_translation(QM_FILES ".." sdmconsole_ru.ts OPTIONS -no-obsolete)
add_custom_target(translations ALL DEPENDS ${QM_FILES})

# Install translations

install(FILES "${QM_FILES}" DESTINATION "${I18N_INSTALL_DIR}")

if(EXISTS ${QTBASE_RU})
	install(FILES "${QTBASE_RU}" DESTINATION "${I18N_INSTALL_DIR}")
else()
	message(WARNING "Can't find Qt5 translations on this system")
endif()
