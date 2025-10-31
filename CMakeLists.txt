cmake_minimum_required(VERSION 3.20)
project(MiniRTS LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-Wno-interference-size)

include(GNUInstallDirs)

add_subdirectory(src)    # defines MiniRTS first
add_subdirectory(bench)  # then benchmarks
add_subdirectory(test)

install(EXPORT MiniRTSTargets
        FILE MiniRTSTargets.cmake
        NAMESPACE MiniRTS::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MiniRTS
)
