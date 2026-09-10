#include "mmu.h"

Mmu::Mmu() {
    reset();
}

void Mmu::reset() {
    memory_.fill(0x00);
}

uint8_t Mmu::read(uint16_t address) const {
    // Echo RAM (mirror of 0xC000 - 0xDDFF)
    if (address >= 0xE000 && address <= 0xFDFF)
        return memory_[address - 0x2000];

    return memory_[address];
}

void Mmu::write(uint16_t address, uint8_t data) {
    if (address >= 0xE000 && address <= 0xFDFF) {
        // Echo RAM
        memory_[address - 0x2000] = data;
        return;
    }

    // TODO: map memory segments to dedicated logic
    memory_[address] = data;
}
