#include <fstream>
#include <iostream>

#include "core/bus.h"

Bus::Bus() {
    memory.fill(0x00);
    ie_register = 0x00;
    if_register = 0xE1;
}

uint8_t Bus::read(uint16_t address) const {
    // External ROM (0x0000-0x7FFF) or RAM (0xA000-0xBFFF)
    if (address < 0x8000 || (address >= 0xA000 && address <= 0xBFFF)) {
        if (cartridge) return cartridge->read(address);
    }

    // PPU
    if ((address >= 0x8000 && address <= 0x9FFF) ||
        (address >= 0xFE00 && address <= 0xFE9F) ||
        (address >= 0xFF40 && address <= 0xFF4B)) {
        if (ppu) return ppu->read(address);
    }

    // Joypad
    if (address == 0xFF00) {
        if (joypad) return joypad->read();
    }

    // Timer (0xFF04 - 0xFF07)
    if (address >= 0xFF04 && address <= 0xFF07) {
        if (timer) return timer->read_reg(address);
    }

    // IF
    if (address == 0xFF0F) {
        return if_register | 0xE0;
    }

    // IE
    if (address == 0xFFFF) {
        return ie_register;
    }

    return memory[address];
}

void Bus::write(uint16_t address, uint8_t value) {
    // External ROM (0x0000-0x7FFF) or RAM (0xA000-0xBFFF)
    if (address < 0x8000 || (address >= 0xA000 && address <= 0xBFFF)) {
        if (cartridge) {
            cartridge->write(address, value);
            return;
        }
    }

    // OAM DMA Transfer
    if (address == 0xFF46) {
        uint16_t source = value << 8;
        for (int i = 0; i < 160; i++) {
            uint8_t data = this->read(source + i);
            if (ppu) ppu->write(0xFE00 + i, data);
        }
        return;
    }

    // PPU
    if ((address >= 0x8000 && address <= 0x9FFF) ||
        (address >= 0xFE00 && address <= 0xFE9F) ||
        (address >= 0xFF40 && address <= 0xFF4B)) {
        if (ppu) {
            ppu->write(address, value);
            return;
        }
    }

    // Joypad
    if (address == 0xFF00) {
        if (joypad) {
            joypad->write(value);
            return;
        }
    }

    // Timer (0xFF04 - 0xFF07)
    if (address >= 0xFF04 && address <= 0xFF07) {
        if (timer) {
            timer->write_reg(address, value);
            return;
        }
    }

    // IF
    if (address == 0xFF0F) {
        if_register = value | 0xE0;
        return;
    }

    // IE
    if (address == 0xFFFF) {
        ie_register = value;
        return;
    }

    memory[address] = value;

    // Blargg's test
    if (address == 0xFF02 && value == 0x81) {
        char c = static_cast<char>(memory[0xFF01]);
        std::cout << c << std::flush;
    }
}

void Bus::request_interrupt(uint8_t bit) {
    if_register |= (1 << bit);
}
