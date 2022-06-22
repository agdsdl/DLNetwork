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


add_subdirectory(util)
add_subdirectory(network)
add_subdirectory(demo)