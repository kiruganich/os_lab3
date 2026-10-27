cmake_minimum_required(VERSION 3.16)

get_filename_component(COMPONENT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${COMPONENT_NAME} C)

add_library(${COMPONENT_NAME} STATIC common.c)

# lets the code write #include "common/common.h"
target_include_directories(${COMPONENT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
