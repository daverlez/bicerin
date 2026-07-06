#pragma once
#include <cstdint>
#include <array>
#include <vector>

class Apu {
public:
    Apu();
    ~Apu() = default;

    /// Reads a byte from audio registers (0xFF10 - 0xFF3F)
    uint8_t read(uint16_t address) const;

    /// Writes a byte from audio registers (0xFF10 - 0xFF3F)
    void write(uint16_t address, uint8_t value);

    void tick(uint8_t m_cycles);

    const std::vector<float>& get_audio_buffer() const { return audio_buffer; }
    void clear_audio_buffer() { audio_buffer.clear(); }

private:
    std::array<uint8_t, 48> registers{};

    uint16_t cycles_accumulator{0};
    uint32_t sample_tracker{0};
    std::vector<float> audio_buffer;
};