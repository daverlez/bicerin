#pragma once
#include <string>

#include "core/cpu.h"
#include "core/bus.h"
#include "core/timer.h"

class System {
public:
    System();
    ~System() = default;

    /// Loads a ROM file.
    bool load_rom(const std::string& filepath);

    /// Starts the continuous emulation loop.
    void run();

private:
    Timer timer;
    Bus bus;
    Cpu cpu;
};