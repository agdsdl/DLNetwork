cmake_minimum_required(VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)

set(PLATFORM_ARM false)

if(DEFINED ENV{TARGET_PLATFORM})
	if($ENV{TARGET_PLATFORM} STREQUAL ARM)
		set(PLATFORM_ARM true)
	endif()
endif()

if(PLATFORM_ARM)
	message(STATUS "env:$ENV{TARGET_PLATFORM}")
	set(CMAKE_SYSTEM_NAME Linux)
	set(CMAKE_SYSTEM_PROCESSOR aarch64)
	set(TOOLCHAIN_DIR /usr)
	set(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/bin/aarch64-linux-gnu-gcc-7)
	set(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/bin/aarch64-linux-gnu-g++-7)
	project(DLNetwork VERSION 0.1.0 LANGUAGES C CXX)
else()
	message(STATUS "env: x86_64")
	project(DLNetwork VERSION 0.1.0 LANGUAGES C CXX)
endif()

if(UNIX AND NOT APPLE)
	# Linux平台启用epoll
	add_definitions(-D_USE_EPOLL_)
endif()

if(WIN32)
	# Windows平台使用wepoll
	# add_definitions(-D_USE_EPOLL_ -D_USE_WEPOLL_ -D_WIN32_)
	add_definitions(-D_WIN32_)
endif()

if(_USE_EPOLL_)
    message(STATUS "EPOLL is enabled")
    # 可以添加相关的编译定义
    add_definitions(-D_USE_EPOLL_=1)
endif()

add_subdirectory(util)
add_subdirectory(network)
add_subdirectory(demo)