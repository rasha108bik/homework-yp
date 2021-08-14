cmake_minimum_required(VERSION 3.17)
project(sprint_7)

set(CMAKE_CXX_STANDARD 17)

SET(CMAKE_C_FLAGS "-O1 -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=address -g -Wall -Werror")
SET(CMAKE_CXX_FLAGS "-O1 -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=address -g -Wall -Werror")

add_executable(sprint_7 main.cpp simple_vector.h array_ptr.h)