cmake_minimum_required(VERSION 4.0)
project(LAB1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(LAB1 main.cpp
        ../LAB001/tools/graph.hpp
        ../LAB001/tests/test_graf.cpp)

enable_testing()


add_subdirectory(../LAB001/Catch2-3.16.0)

add_executable(graf_test
        ../LAB001/tests/test_graf.cpp
        ../LAB001/tools/graph.hpp)

target_link_libraries(graf_test
        PRIVATE
        Catch2::Catch2WithMain
)

target_include_directories(graf_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})