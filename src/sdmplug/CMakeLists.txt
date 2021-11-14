cmake_minimum_required(VERSION 3.3.0)

add_library(sdmplug STATIC src/sdmplugbase.cpp src/sdmplugin.cpp src/sdmdevice.cpp src/sdmchannel.cpp src/sdmsource.cpp)

target_include_directories(sdmplug PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(sdmplug config utils u8e api)

# Note: it seems that *BSD flavors neither have nor need libdl

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_link_libraries(sdmplug dl)
endif()
