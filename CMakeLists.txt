cmake_minimum_required(VERSION 3.16)

project(lab3 C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
# _GNU_SOURCE is needed for pthread_barrier_t and gettid
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -D_GNU_SOURCE")

add_subdirectory(common)
add_subdirectory(os)
