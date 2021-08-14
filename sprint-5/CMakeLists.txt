cmake_minimum_required(VERSION 3.17)
project(yandex-sprint-5)

set(CMAKE_CXX_STANDARD 20)

add_executable(yandex-sprint-5 main.cpp document.cpp document.h paginator.h read_input_functions.cpp read_input_functions.h request_queue.cpp request_queue.h search_server.cpp search_server.h string_processing.cpp string_processing.h log_duration.h remove_duplicates.cpp remove_duplicates.h test_example_functions.cpp test_example_functions.h)