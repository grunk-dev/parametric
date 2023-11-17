cmake_minimum_required(VERSION 3.4)

project(parametric VERSION 0.3.2)

option(BUILD_PARAMETRIC_BENCHMARKS "Build benchmarks using Google Benchmark" OFF)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

add_library(parametric INTERFACE)
target_include_directories(parametric INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(parametric INTERFACE cxx_std_17)

include(GNUInstallDirs)

if(BUILD_PARAMETRIC_BENCHMARKS)
add_subdirectory(benchmarks)
endif()

add_subdirectory(examples)
add_subdirectory(tests)
add_subdirectory(docs)

install(TARGETS parametric
        EXPORT parametric-targets
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY parametric DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

include(CMakePackageConfigHelpers)


configure_package_config_file(
    ${CMAKE_CURRENT_LIST_DIR}/cmake/parametric-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/parametric-config.cmake
INSTALL_DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/parametric
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/parametric-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(
    EXPORT parametric-targets
    FILE parametric-targets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/parametric
)

install (
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/parametric-config-version.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/parametric-config.cmake"
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/parametric
)
