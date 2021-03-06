
cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

# set(CMAKE_AUTOMOC ON)

set(PLATFORM_ARM false)
if(DEFINED ENV{TARGET_PLATFORM})
	if($ENV{TARGET_PLATFORM} STREQUAL ARM)
		set(PLATFORM_ARM true)
	endif()
endif()

# 初步定位：C和Cpp混编时，异常处理会有问题，需要加上这个字段
# set(CMAKE_CXX_FLAGS "-fexceptions")

# source files
file(GLOB_RECURSE SRC_LIST "*.cpp" "*.c" "*.h")

message(STATUS "network projName:network, srouceDir:${PROJECT_SOURCE_DIR}")
add_library(network STATIC
    ${SRC_LIST}
)

target_include_directories(network PRIVATE ../util)

target_include_directories(network PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_compile_definitions(network PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN QT_MESSAGELOGCONTEXT)

target_compile_options(network PRIVATE "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/utf-8>")
target_compile_options(network PRIVATE "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/MP>")
