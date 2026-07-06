add_library(core_lib STATIC
        apu.cpp
        bus.cpp
        cartridge.cpp
        cpu.cpp
        joypad.cpp
        mbc1.cpp
        ppu.cpp
        system.cpp
        timer.cpp
)

target_include_directories(core_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)