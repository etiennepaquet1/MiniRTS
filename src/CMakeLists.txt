# ─────────────────────────────────────────────
# Core Runtime Library
# ─────────────────────────────────────────────
add_library(Runtime
        core/worker.cpp
)

target_include_directories(Runtime
        PUBLIC
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/async
        ${CMAKE_SOURCE_DIR}/external
        ${CMAKE_SOURCE_DIR}/external/WorkStealingQueue/include
        ${CMAKE_SOURCE_DIR}/external/SPSCQueue/include/rigtorp
)
