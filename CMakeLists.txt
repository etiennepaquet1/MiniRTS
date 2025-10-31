cmake_minimum_required(VERSION 3.20)
project(MiniRTS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose build type" FORCE)
endif()

# Common compile options
add_compile_options(-Wno-interference-size)

include(FetchContent)

# ─────────────────────────────────────────────
# Subdirectories
# ─────────────────────────────────────────────
add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(bench)
