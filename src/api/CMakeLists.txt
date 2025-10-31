# ─────────────────────────────────────────────
# Public API headers
# ─────────────────────────────────────────────

# Optional version header generator
# configure_file(version.h.in version.h @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/version.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/minirts)

# Nothing else needed — headers are installed from parent src/CMakeLists
