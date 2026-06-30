add_library(core_lib STATIC
        bus.cpp
        cartridge.cpp
        cpu.cpp
        system.cpp
        timer.cpp
)

target_include_directories(core_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)