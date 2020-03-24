cmake_minimum_required(VERSION 3.4)

project(parametric-examples)

add_executable(tutorial tutorial.cpp)
add_executable(custom_computenode custom_computenode.cpp)
target_link_libraries(tutorial PRIVATE parametric)
target_link_libraries(custom_computenode PRIVATE parametric)
