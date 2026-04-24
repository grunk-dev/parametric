# SPDX-FileCopyrightText: 2026 Jan Kleinert <jan.kleinert@dlr.de>
#
# SPDX-License-Identifier: Apache-2.0

option(PARAMETRIC_BUILD_TESTS "Build tests using Google Test" OFF)

if(PARAMETRIC_BUILD_TESTS)
  cmake_minimum_required(VERSION 3.10)
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

endif()
