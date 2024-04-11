cmake_minimum_required(VERSION 3.4)
project(parametric-tests)

if (NOT TARGET "parametric")
  find_package(parametric REQUIRED)
endif()

if(MULTI_THREADED)
  if(UNIX)
    find_package(TBB REQUIRED COMPONENTS tbb)
  endif(UNIX)
endif(MULTI_THREADED)

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

if(MULTI_THREADED)
  if(UNIX)
  target_link_libraries(runUnitTests PRIVATE tbb)
  target_compile_definitions(runUnitTests PUBLIC MULTI_THREADED=1)
  endif(UNIX)
endif(MULTI_THREADED)

target_compile_features(runUnitTests PRIVATE cxx_std_17)
add_gtest(runUnitTests)
