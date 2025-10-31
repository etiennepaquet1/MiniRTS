cmake_minimum_required(VERSION 3.20)
project(MiniRTS LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-Wno-interference-size)

include(GNUInstallDirs)

add_subdirectory(src)
add_subdirectory(bench)
add_subdirectory(test)
add_subdirectory(example)

install(EXPORT MiniRTSTargets
        FILE MiniRTSTargets.cmake
        NAMESPACE MiniRTS::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MiniRTS
)
