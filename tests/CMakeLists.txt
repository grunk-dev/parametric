cmake_minimum_required(VERSION 3.5)
project(parametric-tests)

if (NOT TARGET "parametric")
  find_package(parametric REQUIRED)
endif()

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
target_compile_features(runUnitTests PRIVATE cxx_std_17)
add_gtest(runUnitTests)
