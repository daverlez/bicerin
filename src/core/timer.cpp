#include "core/timer.h"

Timer::Timer() {
    reset();
}

void Timer::reset() {
    internal_counter = 0x0000;
    tima = 0x00;
    tma = 0x00;
    tac = 0x00;
    interrupt_requested = false;
}

uint16_t Timer::get_clock_divider() const {
    switch (tac & 0x03) {
        case 0x00: return 1024; // 4096 Hz
        case 0x01: return 16;   // 262144 Hz
        case 0x02: return 64;   // 65536 Hz
        case 0x03: return 256;  // 131072 Hz
        default:   return 1024;
    }
}

void Timer::tick(uint8_t m_cycles) {
    uint16_t t_cycles = static_cast<uint16_t>(m_cycles) * 4;

    for (uint16_t i = 0; i < t_cycles; i++) {
        uint16_t divider = get_clock_divider();
        bool old_bit = is_enabled() && ((internal_counter & (divider >> 1)) != 0);

        internal_counter++;

        bool new_bit = is_enabled() && ((internal_counter & (divider >> 1)) != 0);

        if (old_bit && !new_bit) {
            tima++;
            if (tima == 0x00) {
                tima = tma;
                interrupt_requested = true;
            }
        }
    }
}

uint8_t Timer::read_reg(uint16_t address) const {
    switch (address) {
        case 0xFF04: return static_cast<uint8_t>(internal_counter >> 8);
        case 0xFF05: return tima;
        case 0xFF06: return tma;
        case 0xFF07: return tac | 0xF8;
        default:     return 0xFF;
    }
}

void Timer::write_reg(uint16_t address, uint8_t value) {
    switch (address) {
        case 0xFF04:
            internal_counter = 0;
            break;
        case 0xFF05:
            tima = value;
            break;
        case 0xFF06:
            tma = value;
            break;
        case 0xFF07:
            tac = value & 0x07;
            break;
    }
}