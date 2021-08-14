cmake_minimum_required(VERSION 3.17)
project(sprint_12)

set(CMAKE_CXX_STANDARD 17)

add_executable(sprint_12
        domain.cpp
        domain.h
        geo.cpp
        geo.h
        graph.h
        json.cpp
        json.h
        json_builder.cpp
        json_builder.h
        json_reader.cpp
        json_reader.h
        main.cpp
        map_renderer.cpp
        map_renderer.h
        ranges.h
        request_handler.cpp
        request_handler.h
        router.h
        svg.cpp
        svg.h
        transport_catalogue.cpp
        transport_catalogue.h
        transport_router.cpp
        transport_router.h
        )