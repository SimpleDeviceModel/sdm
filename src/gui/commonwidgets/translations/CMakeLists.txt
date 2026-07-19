cmake_minimum_required(VERSION 3.22)

find_package(Qt6LinguistTools)

set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM TRUE)

# Compile commonwidgets translation

qt_create_translation(QM_FILES ".." commonwidgets_ru.ts OPTIONS -no-obsolete)
add_custom_target(commonwidgets_translations ALL DEPENDS "${QM_FILES}")

# Install translations

install(FILES "${QM_FILES}" DESTINATION "${I18N_INSTALL_DIR}")
