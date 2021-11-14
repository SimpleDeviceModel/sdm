cmake_minimum_required(VERSION 3.3.0)

add_library(utils STATIC src/csvparser.cpp src/dirutil.cpp src/ioredirector.cpp src/loadablemodule.cpp src/stringutils.cpp)

target_include_directories(utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(utils u8e)

if(UNIX)
	target_link_libraries(utils pthread)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_link_libraries(utils dl)
endif()

if(WIN32)
	target_link_libraries(utils userenv psapi)
endif()

set_target_properties(utils PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
