cmake_minimum_required(VERSION 3.4)

project(parametric-examples)

if (NOT TARGET "parametric")
  find_package(parametric REQUIRED)
endif()

add_executable(tutorial tutorial.cpp)
add_executable(custom_computenode custom_computenode.cpp)
add_executable(parametric_struct parametric_struct.cpp)
target_link_libraries(tutorial PRIVATE parametric)
target_link_libraries(custom_computenode PRIVATE parametric)
target_link_libraries(parametric_struct PRIVATE parametric)

# a CAD example
find_package(OCE QUIET)
if (OCE_FOUND)
    add_executable(parametric_cad parametric_cad.cpp)
    target_link_libraries(parametric_cad PRIVATE parametric TKBO TKPrim)
    target_include_directories(parametric_cad PRIVATE ${OCE_INCLUDE_DIRS})
else (OCE_FOUND)
    message(STATUS "OCE not found. Parametric CAD example cannot be build.")
endif(OCE_FOUND)
