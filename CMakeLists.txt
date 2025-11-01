cmake_minimum_required(VERSION 3.20)
project(MiniRTS LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-Wno-interference-size)

include(GNUInstallDirs)

option(BUILD_TESTS "Build unit tests" ON)

if (BUILD_TESTS)
    include(CTest)
    enable_testing()
endif()

# Sanitizer configuration
option(ENABLE_ASAN "Enable AddressSanitizer and UBSan" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ENABLE_MSAN "Enable MemorySanitizer" OFF)

if(ENABLE_ASAN)
    message(STATUS "Building with AddressSanitizer + UndefinedBehaviorSanitizer")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
elseif(ENABLE_TSAN)
    message(STATUS "Building with ThreadSanitizer")
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=thread)
elseif(ENABLE_MSAN)
    message(STATUS "Building with MemorySanitizer")
    add_compile_options(-fsanitize=memory -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=memory)
endif()

if (ENABLE_TSAN)
    set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)  # Disable LTO policy override
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
endif()

if (ENABLE_TSAN)
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer -O1 -fno-lto -fno-fat-lto-objects)
    add_link_options(-fsanitize=thread -fno-lto -fno-fat-lto-objects)
endif()


add_subdirectory(src)
add_subdirectory(bench)
add_subdirectory(test)
add_subdirectory(example)

install(EXPORT MiniRTSTargets
        FILE MiniRTSTargets.cmake
        NAMESPACE MiniRTS::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MiniRTS
)
