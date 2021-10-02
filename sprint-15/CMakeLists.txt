cmake_minimum_required(VERSION 3.20)
project(sprint_15)

set(CMAKE_CXX_STANDARD 17)

add_executable(sprint_15
        lexer.cpp
        lexer.h
        lexer_test_open.cpp
        main.cpp
        parse.cpp
        parse.h
        parse_test.cpp
        runtime.cpp
        runtime.h
        runtime_test.cpp
        statement.cpp
        statement.h
        statement_test.cpp
        test_runner_p.h)
