add_library(core_lib STATIC
        bus.cpp
        cpu.cpp
        timer.cpp
)

target_include_directories(core_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)