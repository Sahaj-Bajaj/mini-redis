cmake_minimum_required(VERSION 3.20)
project(miniredis LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CACHE_WARNING_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

find_package(Threads REQUIRED)

add_library(miniredis_net
    src/net/Socket.cpp
    src/net/TcpListener.cpp
    src/net/ThreadPool.cpp
)

target_include_directories(miniredis_net PUBLIC include)
target_link_libraries(miniredis_net PUBLIC Threads::Threads)
target_compile_options(miniredis_net PRIVATE ${CACHE_WARNING_FLAGS})

add_library(miniredis_protocol
    src/protocol/CommandParser.cpp
)

target_include_directories(miniredis_protocol PUBLIC include)
target_compile_options(miniredis_protocol PRIVATE ${CACHE_WARNING_FLAGS})

add_library(miniredis_store
    src/store/KvStore.cpp
)

target_include_directories(miniredis_store PUBLIC include)
target_link_libraries(miniredis_store PUBLIC Threads::Threads)
target_compile_options(miniredis_store PRIVATE ${CACHE_WARNING_FLAGS})

# Phase 7
add_library(miniredis_stats
    src/stats/Stats.cpp
)

target_include_directories(miniredis_stats PUBLIC include)
target_compile_options(miniredis_stats PRIVATE ${CACHE_WARNING_FLAGS})

add_executable(miniredis
    src/main.cpp
)

target_link_libraries(miniredis PRIVATE
    miniredis_net
    miniredis_protocol
    miniredis_store
    miniredis_stats
)

target_compile_options(miniredis PRIVATE ${CACHE_WARNING_FLAGS})

# ── Benchmark ──────────────────────────────────────────────────────────────────
add_executable(miniredis_bench
    bench/bench.cpp
)

target_include_directories(miniredis_bench PRIVATE include)
target_link_libraries(miniredis_bench PRIVATE Threads::Threads)
target_compile_options(miniredis_bench PRIVATE ${CACHE_WARNING_FLAGS})

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(miniredis_tests
    tests/test_kv_store.cpp
    tests/test_command_parser.cpp
)

target_link_libraries(miniredis_tests PRIVATE
    miniredis_protocol
    miniredis_store
    miniredis_stats
    GTest::gtest_main
)

target_compile_options(miniredis_tests PRIVATE ${CACHE_WARNING_FLAGS})

include(GoogleTest)
gtest_discover_tests(miniredis_tests)