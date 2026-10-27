cmake_minimum_required(VERSION 3.16)

get_filename_component(COMPONENT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${COMPONENT_NAME} C)

find_package(Threads REQUIRED)

add_library(${COMPONENT_NAME} STATIC os_unix.c)

target_include_directories(${COMPONENT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(${COMPONENT_NAME} PUBLIC Threads::Threads)
