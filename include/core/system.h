#pragma once
#include <string>

#include "core/cpu.h"
#include "core/bus.h"
#include "core/timer.h"

class System {
public:
    System(const std::string& rom_path);
    ~System() = default;

    /// Loads a ROM file.
    bool load_rom(const std::string& filepath);

    /// Starts the continuous emulation loop.
    void run_frame();

    void press_button(Joypad::Button button) { joypad.press(button); }
    void release_button(Joypad::Button button) { joypad.release(button); }

    const std::array<uint32_t, 160 * 144>& get_frame_buffer() const { return ppu.get_frame_buffer(); }
    const std::vector<float>& get_audio_buffer() const { return apu.get_audio_buffer(); }
    void clear_audio_buffer() { apu.clear_audio_buffer(); }

    /// Sets the palette for non-color games
    void set_palette(const std::array<uint32_t, 4>& colors) { ppu.set_palette(colors); }

private:
    std::unique_ptr<Cartridge> cartridge;
    Timer timer;
    Bus bus;
    Cpu cpu;
    Joypad joypad;
    Ppu ppu;
    Apu apu;
};