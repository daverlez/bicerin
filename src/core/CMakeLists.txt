add_library(bicerin_core STATIC
        cpu/cpu.cpp
)

target_include_directories(bicerin_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})