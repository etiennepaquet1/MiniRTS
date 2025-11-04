cmake_minimum_required(VERSION 3.20)
project(MiniRTS LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-Wno-interference-size)

include(GNUInstallDirs)

# ─────────────────────────────────────────────
# Core MiniRTS library Definition
# ─────────────────────────────────────────────
# Define the library target here, at the top level.
# It starts as an empty container for sources and properties.
add_library(MiniRTS)

set_target_properties(MiniRTS PROPERTIES
        OUTPUT_NAME minirts
        EXPORT_NAME Runtime
)

# ─────────────────────────────────────────────
# Include directories (Now with clean paths)
# ─────────────────────────────────────────────
# Set properties on the target using paths relative to the project root.
target_include_directories(MiniRTS
        PUBLIC
        # Paths are now clean, relative to THIS file's location (the project root)
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/api>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/core>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/async>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/ConcurrentDeque/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/SPSCQueue/include>

        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Now, descend into the subdirectories. The 'src' directory will add
# its source files to the 'MiniRTS' target we just created.
add_subdirectory(src)
add_subdirectory(bench)
add_subdirectory(test)
add_subdirectory(example)


# ─────────────────────────────────────────────
# Installation (Moved from /src for centralization)
# ─────────────────────────────────────────────
install(TARGETS MiniRTS
        EXPORT MiniRTSTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install the export configuration that defines the MiniRTS:: namespace
install(EXPORT MiniRTSTargets
        FILE MiniRTSTargets.cmake
        NAMESPACE MiniRTS::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MiniRTS
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/api/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/minirts)


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


install(EXPORT MiniRTSTargets
        FILE MiniRTSTargets.cmake
        NAMESPACE MiniRTS::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MiniRTS
)
