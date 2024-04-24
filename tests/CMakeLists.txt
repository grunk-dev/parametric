cmake_minimum_required(VERSION 3.4)
project(parametric-tests)

if (NOT TARGET "parametric")
  find_package(parametric REQUIRED)
endif()

find_package(OpenMP)

enable_testing()
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(GOOGLE_TEST_INDIVIDUAL ON)
include(AddGoogleTest)

set(TESTSOURCES
  testmain.cpp
  dag.cpp
  eval.cpp
  customclass.cpp
  invalidator.cpp
  param.cpp
  parametric_struct.cpp
  serialize.cpp
)

add_executable(runUnitTests ${TESTSOURCES})
target_link_libraries(runUnitTests PRIVATE parametric gtest_main)
if (OpenMP_CXX_FOUND)
  target_link_libraries(runUnitTests PRIVATE OpenMP::OpenMP_CXX)
  if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -openmp:llvm -openmp:experimental")
  endif()
  if (CMAKE_CXX_COMPILER_ID MATCHES "CLANG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp=libomp")
  endif()
endif()

target_compile_features(runUnitTests PRIVATE cxx_std_17)
add_gtest(runUnitTests)
