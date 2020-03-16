cmake_minimum_required(VERSION 3.4)

project(Parametric VERSION 0.0.1)

add_library(parametric INTERFACE)
target_compile_features(parametric INTERFACE cxx_std_14)


add_executable(example main.cpp)
target_link_libraries(example PRIVATE parametric)


install(TARGETS parametric
        EXPORT parametric-targets
        INCLUDES DESTINATION include
)

install(DIRECTORY parametric DESTINATION include)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/parametric-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(
    EXPORT parametric-targets
    FILE parametric.cmake
    NAMESPACE parametric::
    DESTINATION lib/cmake/parametric
)

install (FILES
  "${CMAKE_CURRENT_BINARY_DIR}/parametric-config-version.cmake"
  DESTINATION lib/cmake/parametric)

target_include_directories(parametric INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
