cmake_minimum_required(VERSION 3.4)

project(parametric-examples)

add_executable(tutorial tutorial.cpp)
target_link_libraries(tutorial PRIVATE parametric)