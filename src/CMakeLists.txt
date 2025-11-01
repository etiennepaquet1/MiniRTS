# ─────────────────────────────────────────────
# Core MiniRTS library
# ─────────────────────────────────────────────

add_library(MiniRTS)

# ─────────────────────────────────────────────
# Include directories
# ─────────────────────────────────────────────
target_include_directories(MiniRTS
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/api>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/core>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/async>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external/ConcurrentDeque/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external/ConcurrentDeque/include/riften>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external/SPSCQueue/include/rigtorp>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
add_subdirectory(core)
add_subdirectory(async)
add_subdirectory(api)

set_target_properties(MiniRTS PROPERTIES
        OUTPUT_NAME minirts
        EXPORT_NAME Runtime
)

# ─────────────────────────────────────────────
# Installation
# ─────────────────────────────────────────────
install(TARGETS MiniRTS
        EXPORT MiniRTSTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install public headers under include/minirts/
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/api/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/minirts)
