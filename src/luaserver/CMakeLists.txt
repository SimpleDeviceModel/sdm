cmake_minimum_required(VERSION 3.3.0)

add_library(luaserver STATIC src/luaserver.cpp src/stackguard.cpp src/luacallbackobject.cpp src/luavalue.cpp src/luaconsole.cpp src/luastreamreader.cpp src/luaiterator.cpp)

target_include_directories(luaserver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Ensure that stackguard.cpp is not optimized

if(GCC_CMDLINE_SYNTAX)
	set_source_files_properties(src/stackguard.cpp PROPERTIES COMPILE_FLAGS "-O0")
elseif(MSVC)
	set_source_files_properties(src/stackguard.cpp PROPERTIES COMPILE_FLAGS "/Od")
endif()

# Ensure that whatever is linked to this library uses the same optional compiler features

if(NOT NOEXCEPT_PRESENT EQUAL -1)
	target_compile_definitions(luaserver INTERFACE -DHAVE_NOEXCEPT)
endif()

if(NOT REF_QUALIFIERS_PRESENT EQUAL -1)
	target_compile_definitions(luaserver INTERFACE -DHAVE_REF_QUALIFIERS)
endif()

set_target_properties(luaserver PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_link_libraries(luaserver lua utils)

if(UNIX)
	target_link_libraries(luaserver pthread)
endif()
