#pragma once
#include <cstdint>
#include <array>

#include "timer.h"

class Bus {
public:
    explicit Bus(Timer& t);
    ~Bus() = default;

    /// Reads a byte at a given 16-bit register.
    uint8_t read(uint16_t address) const;

    /// Sets a bit in the IF (0xFF0F) register to request an interrupt.
    void request_interrupt(uint8_t bit);

    /// Writes a byte at a given 16-bit register.
    void write(uint16_t address, uint8_t value);
    
    /// Loads a ROM file in memory.
    bool load_rom(const std::string& filepath);

private:
    Timer& timer;

    std::array<uint8_t, 0x10000> memory{};  // 64 KB
    uint8_t ie_register{0}; // 0xFFFF - Interrupt Enable
    uint8_t if_register{0}; // 0xFF0F - Interrupt Flag
};
