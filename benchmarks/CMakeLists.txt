option(BUILD_PARAMETRIC_BENCHMARKS "Build benchmarks using Google Benchmark" OFF)

if(BUILD_PARAMETRIC_BENCHMARKS)

    project(parametric-benchmarks)


    # download googlebenchmark as thirdparty dependency
    include(FetchContent)
    FetchContent_Declare(googlebenchmark
            GIT_REPOSITORY https://github.com/google/benchmark.git
            GIT_TAG "v1.5.2"
    )
    set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON)
    set(BENCHMARK_ENABLE_TESTING OFF)
    FetchContent_MakeAvailable(googlebenchmark)

    if (NOT TARGET "parametric")
        find_package(parametric CONFIG REQUIRED)
    endif()

    set(BENCHMARKS
        main.cpp
        bench_operators.cpp
    )

    add_executable(benchmarks ${BENCHMARKS})
    target_include_directories(benchmarks PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(benchmarks PRIVATE parametric benchmark::benchmark)
    target_compile_options(benchmarks PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>: -Wall -Wextra -Wshadow --pedantic-errors -Wno-deprecated-copy>
        $<$<CXX_COMPILER_ID:MSVC>: /W4 /WX>)

endif()
