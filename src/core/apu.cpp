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
}

void Apu::tick(uint8_t m_cycles) {
    cycles_accumulator += m_cycles;

    // TODO: frame sequencer
}