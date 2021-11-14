cmake_minimum_required(VERSION 3.3.0)

set(U8E_SRC src/u8efile.cpp src/u8eenv.cpp)

if(WIN32)
	list(APPEND U8E_SRC src/win32/u8ecodec.cpp src/win32/u8eio.cpp)
else()
	list(APPEND U8E_SRC src/posix/u8ecodec.cpp src/posix/u8eio.cpp)
endif()

add_library(u8e STATIC ${U8E_SRC})

target_include_directories(u8e PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(u8e PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

if(UNIX)
	find_library(LIB_ICONV iconv)
# Some platforms have Libiconv already integrated in Libc
	if(LIB_ICONV)
		target_link_libraries(u8e ${LIB_ICONV})
	endif()
endif()

enable_testing()

add_subdirectory(tests)
