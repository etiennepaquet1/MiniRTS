# ─────────────────────────────────────────────
# GoogleTest
# ─────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(MiniRTS_tests
        test_runtime.cpp
        test_future.cpp
        test_when_all.cpp
        test_when_any.cpp
)

target_link_libraries(MiniRTS_tests
        PRIVATE
        gtest_main
        MiniRTS
)

if (ENABLE_TSAN OR ENABLE_ASAN OR ENABLE_MSAN)
    message(STATUS "Sanitizer build: skipping gtest_discover_tests; registering binary as a single test")
    add_test(NAME MiniRTS_tests COMMAND MiniRTS_tests)
else()
    include(GoogleTest)
    gtest_discover_tests(MiniRTS_tests)
endif()
