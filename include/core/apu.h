#pragma once
#include <cstdint>
#include <array>

class Apu {
public:
    Apu();
    ~Apu() = default;

    /// Reads a byte from audio registers (0xFF10 - 0xFF3F)
    uint8_t read(uint16_t address) const;

    /// Writes a byte from audio registers (0xFF10 - 0xFF3F)
    void write(uint16_t address, uint8_t value);

    void tick(uint8_t m_cycles);

private:
    std::array<uint8_t, 48> registers{};
    
    uint16_t cycles_accumulator{0};
};