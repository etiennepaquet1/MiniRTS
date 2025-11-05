# src/api/CMakeLists.txt
add_library(MiniRTS_api INTERFACE
        ../async/when_any.h)
target_include_directories(MiniRTS_api
        INTERFACE
        ${CMAKE_SOURCE_DIR}/src/api
)
target_link_libraries(MiniRTS_api INTERFACE MiniRTS)
