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

    struct PulseChannel {
        bool enabled{false};
        uint16_t timer{0};
        uint8_t duty_step{0};
        uint16_t frequency{0};
        uint8_t volume{0};
        uint8_t duty{0};

        // Length
        uint16_t length_counter{0};
        bool length_enabled{false};

        // Volume Envelope
        uint8_t envelope_timer{0};
        uint8_t envelope_period{0};
        bool envelope_increase{false};

        // Sweep
        uint8_t sweep_timer{0};
        uint8_t sweep_period{0};
        uint8_t sweep_direction{0};
        uint8_t sweep_shift{0};
        bool sweep_enabled{false};
        uint16_t shadow_frequency{0};
    };

    struct WaveChannel {
        bool enabled{false};
        uint16_t timer{0};
        uint8_t wave_step{0};
        uint16_t frequency{0};
        uint8_t volume_code{0};

        uint16_t length_counter{0};
        bool length_enabled{false};
    };

    struct NoiseChannel {
        bool enabled{false};
        uint16_t timer{0};
        uint16_t lfsr{0x7FFF};
        uint8_t volume{0};

        uint16_t length_counter{0};
        bool length_enabled{false};

        uint8_t envelope_timer{0};
        uint8_t envelope_period{0};
        bool envelope_increase{false};

        uint8_t shift_clock{0};
        bool width_mode{false};
        uint8_t divisor_code{0};
    };

    PulseChannel channel1;
    PulseChannel channel2;
    WaveChannel channel3;
    NoiseChannel channel4;

    const std::array<std::array<uint8_t, 8>, 4> duty_table = {{
        {0, 0, 0, 0, 0, 0, 0, 1}, // 12.5%
        {1, 0, 0, 0, 0, 0, 0, 1}, // 25%
        {1, 0, 0, 0, 0, 1, 1, 1}, // 50%
        {0, 1, 1, 1, 1, 1, 1, 0}  // 75%
    }};

    uint16_t cycles_accumulator{0};
    uint8_t frame_sequencer_step{0};

    uint32_t sample_tracker{0};
    std::vector<float> audio_buffer;

    void clock_length();
    void clock_sweep();
    void clock_volume();
};