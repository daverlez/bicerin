#pragma once
#include <cstdint>
#include <array>

class Bus {
public:
    Bus();
    ~Bus() = default;

    /// Reads a byte at a given 16-bit register.
    uint8_t read(uint16_t address) const;

    /// Writes a byte at a given 16-bit register.
    void write(uint16_t address, uint8_t value);

private:
    std::array<uint8_t, 0x10000> memory{};  // 64 KB
};