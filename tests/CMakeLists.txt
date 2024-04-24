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
  set(PARAMETRIC_WITH_OPENMP TRUE)
  if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    if (MSVC_VERSION VERSION_GREATER_EQUAL 1929)
      message(STATUS "Setting openmp:llvm and openmp:experimental to enable OpenMP 3.1 features.")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -openmp:llvm -openmp:experimental")
    else()
      set(PARAMETRIC_WITH_OPENMP FALSE)
      message(STATUS "MSVC version does not support OpenMP 3.1. Not building with OpenMP support.")
    endif()
  endif()
  if (CMAKE_CXX_COMPILER_ID MATCHES "CLANG")
    message(STATUS "Setting -fopenmp=libomp to enable OpenMP 3.1 features.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp=libomp")
  endif()
  if (PARAMETRIC_WITH_OPENMP)
    add_compile_definitions(PARAMETRIC_WITH_OPENMP)
  endif()
endif()

if (PARAMETRIC_WITH_OPENMP)
  target_link_libraries(runUnitTests PRIVATE OpenMP::OpenMP_CXX)
endif()

target_compile_features(runUnitTests PRIVATE cxx_std_17)
add_gtest(runUnitTests)
