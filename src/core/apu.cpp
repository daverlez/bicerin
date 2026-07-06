#include "core/apu.h"

Apu::Apu() {
    registers.fill(0x00);
}

uint8_t Apu::read(uint16_t address) const {
    if (address >= 0xFF10 && address <= 0xFF3F)
        return registers[address - 0xFF10];

    return 0xFF;
}

void Apu::write(uint16_t address, uint8_t value) {
    if (address >= 0xFF10 && address <= 0xFF3F)
        registers[address - 0xFF10] = value;

    // Channel 1 length (NR11)
    if (address == 0xFF11)
        channel1.length_counter = 64 - (value & 0x3F);

    // Channel 1 (write in NR14)
    if (address == 0xFF14 && (value & 0x80)) {
        channel1.enabled = true;
        channel1.frequency = ((value & 0x07) << 8) | registers[0xFF13 - 0xFF10];
        channel1.timer = (2048 - channel1.frequency);
        channel1.duty = registers[0xFF11 - 0xFF10] >> 6;

        // Envelope (NR12)
        uint8_t nr12 = registers[0xFF12 - 0xFF10];
        channel1.volume = nr12 >> 4;
        channel1.envelope_increase = (nr12 & 0x08) != 0;
        channel1.envelope_period = nr12 & 0x07;
        channel1.envelope_timer = channel1.envelope_period > 0 ? channel1.envelope_period : 8;

        // Length (NR14)
        channel1.length_enabled = (value & 0x40) != 0;
        if (channel1.length_counter == 0) channel1.length_counter = 64;
    }

    // Channel 2 length (NR21)
    if (address == 0xFF16)
        channel2.length_counter = 64 - (value & 0x3F);

    // Channel 2 trigger (NR24)
    if (address == 0xFF19 && (value & 0x80)) {
        channel2.enabled = true;
        channel2.frequency = ((value & 0x07) << 8) | registers[0xFF18 - 0xFF10];
        channel2.timer = (2048 - channel2.frequency);
        channel2.duty = registers[0xFF16 - 0xFF10] >> 6;

        // Envelope (NR22)
        uint8_t nr22 = registers[0xFF17 - 0xFF10];
        channel2.volume = nr22 >> 4;
        channel2.envelope_increase = (nr22 & 0x08) != 0;
        channel2.envelope_period = nr22 & 0x07;
        channel2.envelope_timer = channel2.envelope_period > 0 ? channel2.envelope_period : 8;

        // Length (NR24)
        channel2.length_enabled = (value & 0x40) != 0;
        if (channel2.length_counter == 0) channel2.length_counter = 64;
    }

    // Channel 3 length (NR31)
    if (address == 0xFF1B)
        channel3.length_counter = 256 - value;

    // Channel 3 trigger (NR34)
    if (address == 0xFF1E && (value & 0x80)) {
        bool dac_enabled = (registers[0xFF1A - 0xFF10] & 0x80) != 0;

        if (dac_enabled)
            channel3.enabled = true;

        channel3.frequency = ((value & 0x07) << 8) | registers[0xFF1D - 0xFF10];
        channel3.timer = (2048 - channel3.frequency);
        channel3.wave_step = 0;
        channel3.volume_code = (registers[0xFF1C - 0xFF10] >> 5) & 0x03;
        channel3.length_enabled = (value & 0x40) != 0;
        if (channel3.length_counter == 0) channel3.length_counter = 256;
    }
}

void Apu::tick(uint8_t m_cycles) {
    cycles_accumulator += m_cycles;

    // Frame sequencer
    // 1048576 M-Cycles / 512 Hz = 2048 M-Cycles
    if (cycles_accumulator >= 2048) {
        cycles_accumulator -= 2048;

        switch (frame_sequencer_step) {
            case 0: clock_length(); break;
            case 1: break;
            case 2: clock_length(); clock_sweep(); break;
            case 3: break;
            case 4: clock_length(); break;
            case 5: break;
            case 6: clock_length(); clock_sweep(); break;
            case 7: clock_volume(); break;
        }

        frame_sequencer_step = (frame_sequencer_step + 1) % 8;
    }

    if (channel1.enabled) {
        if (channel1.timer <= m_cycles) {
            channel1.timer = (2048 - channel1.frequency);
            channel1.duty_step = (channel1.duty_step + 1) % 8;
        } else {
            channel1.timer -= m_cycles;
        }
    }

    if (channel2.enabled) {
        if (channel2.timer <= m_cycles) {
            channel2.timer = (2048 - channel2.frequency);
            channel2.duty_step = (channel2.duty_step + 1) % 8;
        } else {
            channel2.timer -= m_cycles;
        }
    }

    if (channel3.enabled) {
        if (channel3.timer <= m_cycles * 2) {
            channel3.timer = (2048 - channel3.frequency);
            channel3.wave_step = (channel3.wave_step + 1) % 32;
        } else {
            channel3.timer -= m_cycles * 2;
        }
    }

    // Sampling synchronization
    sample_tracker += m_cycles * 44100;

    while (sample_tracker >= 1048576) {
        sample_tracker -= 1048576;

        float ch1_out = 0.0f;
        if (channel1.enabled && channel1.volume > 0) {
            uint8_t amplitude = duty_table[channel1.duty][channel1.duty_step];
            ch1_out = (amplitude * channel1.volume) / 15.0f;
        }

        float ch2_out = 0.0f;
        if (channel2.enabled && channel2.volume > 0) {
            uint8_t amplitude = duty_table[channel2.duty][channel2.duty_step];
            ch2_out = (amplitude * channel2.volume) / 15.0f;
        }

        float ch3_out = 0.0f;
        if (channel3.enabled && (registers[0xFF1A - 0xFF10] & 0x80)) {
            uint8_t wave_byte = registers[0x20 + (channel3.wave_step / 2)];

            uint8_t sample = 0;
            if (channel3.wave_step % 2 == 0)
                sample = wave_byte >> 4;
            else
                sample = wave_byte & 0x0F;

            if (channel3.volume_code == 0) {
                sample = 0;         // Mute
            } else if (channel3.volume_code == 1) {
                                    // 100%
            } else if (channel3.volume_code == 2) {
                sample >>= 1;       // 50%
            } else if (channel3.volume_code == 3) {
                sample >>= 2;       // 25%
            }

            ch3_out = sample / 15.0f;
        }

        float mix = (ch1_out + ch2_out + ch3_out) * 0.1f;

        // TODO: add up channels 3 and 4

        audio_buffer.push_back(mix);    // Left
        audio_buffer.push_back(mix);    // Right
    }
}

void Apu::clock_length() {
    if (channel1.length_enabled && channel1.length_counter > 0) {
        channel1.length_counter--;
        if (channel1.length_counter == 0) {
            channel1.enabled = false;
        }
    }

    if (channel2.length_enabled && channel2.length_counter > 0) {
        channel2.length_counter--;
        if (channel2.length_counter == 0) {
            channel2.enabled = false;
        }
    }

    if (channel3.length_enabled && channel3.length_counter > 0) {
        channel3.length_counter--;
        if (channel3.length_counter == 0) {
            channel3.enabled = false;
        }
    }
}

void Apu::clock_sweep() {
    // TODO
}

void Apu::clock_volume() {
    auto process_envelope = [](PulseChannel& channel) {
        if (channel.envelope_period > 0) {
            channel.envelope_timer--;
            if (channel.envelope_timer == 0) {
                channel.envelope_timer = channel.envelope_period;

                if (channel.envelope_increase && channel.volume < 15)
                    channel.volume++;
                else if (!channel.envelope_increase && channel.volume > 0)
                    channel.volume--;
            }
        }
    };

    process_envelope(channel1);
    process_envelope(channel2);
}