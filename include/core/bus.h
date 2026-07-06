#pragma once
#include <cstdint>
#include <array>

#include "cartridge.h"
#include "joypad.h"
#include "ppu.h"
#include "timer.h"

class Bus {
public:
    Bus();
    ~Bus() = default;

    /// Reads a byte at a given 16-bit register.
    uint8_t read(uint16_t address) const;

    /// Sets a bit in the IF (0xFF0F) register to request an interrupt.
    void request_interrupt(uint8_t bit);

    /// Writes a byte at a given 16-bit register.
    void write(uint16_t address, uint8_t value);

    void connect_timer(Timer* t) { timer = t; }
    void connect_cartridge(Cartridge* c) { cartridge = c; }
    void connect_joypad(Joypad* j) { joypad = j; }
    void connect_ppu(Ppu* p) { ppu = p; }

private:
    Timer* timer{nullptr};
    Cartridge* cartridge{nullptr};
    Joypad* joypad{nullptr};
    Ppu* ppu{nullptr};

    std::array<uint8_t, 0x10000> memory{};  // 64 KB
    uint8_t ie_register{0}; // 0xFFFF - Interrupt Enable
    uint8_t if_register{0}; // 0xFF0F - Interrupt Flag
};
