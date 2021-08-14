cmake_minimum_required(VERSION 3.17)
project(sprint-11)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)

add_executable(project
        domain.cpp
        domain.h
        geo.cpp
        geo.h
        json.cpp
        json.h
        json_builder.cpp
        json_builder.h
        json_reader.cpp
        json_reader.h
        main.cpp
        map_renderer.cpp
        map_renderer.h
        request_handler.cpp
        request_handler.h
        svg.cpp
        svg.h
        transport_catalogue.cpp
        transport_catalogue.h)
