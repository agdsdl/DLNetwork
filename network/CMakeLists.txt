cmake_minimum_required(VERSION 3.8)
set(THREADS_PREFER_PTHREAD_FLAG ON)

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

find_package(Threads REQUIRED)
target_link_libraries(network PUBLIC Threads::Threads)
target_compile_options(network PUBLIC "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/wd4819>")
target_compile_options(network PUBLIC "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/wd4828>")

if(ENABLE_OPENSSL)
find_package(OpenSSL REQUIRED)
target_link_libraries(network PUBLIC OpenSSL::SSL OpenSSL::Crypto)
target_compile_definitions(network PUBLIC ENABLE_OPENSSL)
endif()

target_include_directories(network PRIVATE ../util)
# target_link_libraries(network PUBLIC util)

target_include_directories(network PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_compile_definitions(network PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN QT_MESSAGELOGCONTEXT)

target_compile_options(network PRIVATE "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/utf-8>")
target_compile_options(network PRIVATE "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/MP>")
