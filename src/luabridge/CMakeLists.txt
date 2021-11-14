cmake_minimum_required(VERSION 3.3.0)

add_library(luabridge STATIC src/luabridge.cpp src/luatextcodec.cpp)

target_include_directories(luabridge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(luabridge config luaserver sdmplug u8e utils)
