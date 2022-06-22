cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_AUTOMOC ON)

file(GLOB_RECURSE SRC_LIST "*.cpp" "*.c" "*.h")

add_executable(${PROJECT_NAME}
    ${SRC_LIST}
)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/util)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/network)

target_link_libraries(${PROJECT_NAME} PRIVATE network)
target_link_libraries(${PROJECT_NAME} PRIVATE util)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    OBJECTS DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
