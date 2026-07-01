cmake_minimum_required(VERSION 3.20)

project(MiniRedis LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Scoped to our own targets (not global) so future third-party targets
# aren't held to our warning level.
set(CACHE_WARNING_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

add_library(miniredis_net
    src/net/Socket.cpp
    src/net/TcpListener.cpp
)

target_include_directories(miniredis_net PUBLIC include)
target_compile_options(miniredis_net PRIVATE ${CACHE_WARNING_FLAGS})

add_executable(miniredis
    src/main.cpp
)

target_link_libraries(miniredis PRIVATE miniredis_net)
target_compile_options(miniredis PRIVATE ${CACHE_WARNING_FLAGS})