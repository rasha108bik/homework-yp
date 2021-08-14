cmake_minimum_required(VERSION 3.17)
project(sprint_9)

set(CMAKE_CXX_STANDARD 17)

add_executable(geo main.cpp geo.h input_reader.h stat_reader.h transport_catalogue.h transport_catalogue.cpp input_reader.cpp stat_reader.cpp)