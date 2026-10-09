cmake_minimum_required(VERSION 3.20)
project(portalcraft_source_bridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(portalcraft_bridge SHARED
    src/portalcraft_plugin.cpp
    src/shared_memory_win.cpp
)

target_include_directories(portalcraft_bridge PRIVATE include)
target_compile_definitions(portalcraft_bridge PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
