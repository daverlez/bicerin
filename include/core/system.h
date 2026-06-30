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
    void run();

private:
    std::unique_ptr<Cartridge> cartridge;
    Timer timer;
    Bus bus;
    Cpu cpu;
};