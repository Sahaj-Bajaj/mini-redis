cmake_minimum_required(VERSION 3.20)

project(MiniRedis LANGUAGES CXX)

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

# -----------------------------------------------------------------------------
# Networking library
# -----------------------------------------------------------------------------

add_library(miniredis_net
    src/net/Socket.cpp
    src/net/TcpListener.cpp
)

target_include_directories(miniredis_net PUBLIC include)
target_compile_options(miniredis_net PRIVATE ${CACHE_WARNING_FLAGS})

# -----------------------------------------------------------------------------
# Protocol library
# -----------------------------------------------------------------------------

add_library(miniredis_protocol
    src/protocol/CommandParser.cpp
)

target_include_directories(miniredis_protocol PUBLIC include)
target_compile_options(miniredis_protocol PRIVATE ${CACHE_WARNING_FLAGS})

# -----------------------------------------------------------------------------
# Store library
# -----------------------------------------------------------------------------

add_library(miniredis_store
    src/store/KvStore.cpp
)

target_include_directories(miniredis_store PUBLIC include)
target_compile_options(miniredis_store PRIVATE ${CACHE_WARNING_FLAGS})

# -----------------------------------------------------------------------------
# Server
# -----------------------------------------------------------------------------

add_executable(miniredis
    src/main.cpp
)

target_link_libraries(miniredis PRIVATE
    miniredis_net
    miniredis_protocol
    miniredis_store
)

target_compile_options(miniredis PRIVATE ${CACHE_WARNING_FLAGS})

# -----------------------------------------------------------------------------
# GoogleTest
# -----------------------------------------------------------------------------

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
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
    GTest::gtest_main
)

target_compile_options(miniredis_tests PRIVATE ${CACHE_WARNING_FLAGS})

include(GoogleTest)

gtest_discover_tests(miniredis_tests)