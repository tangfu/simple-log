project(simplelog)
cmake_minimum_required(VERSION 2.8.4)


add_definitions("-g -Wall")
add_subdirectory(src)


export(PACKAGE simplelog)
export(TARGETS simplelog FILE simplelog-exports.cmake)
configure_file(simplelog-config.cmake.in ${CMAKE_BINARY_DIR}/simplelog-config.cmake)
