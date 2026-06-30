#pragma once
#include <cstdint>
#include <array>

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

    Timer timer;    // TODO: move to System class when implemented

private:
    std::array<uint8_t, 0x10000> memory{};  // 64 KB

    uint8_t ie_register{0}; // 0xFFFF - Interrupt Enable
    uint8_t if_register{0}; // 0xFF0F - Interrupt Flag
};
