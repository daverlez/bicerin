#include <fstream>
#include <iostream>

#include "core/bus.h"

Bus::Bus(Timer& t) : timer(t) {
    memory.fill(0x00);
    ie_register = 0x00;
    if_register = 0xE1;
}

uint8_t Bus::read(uint16_t address) const {
    // Timer (0xFF04 - 0xFF07)
    if (address >= 0xFF04 && address <= 0xFF07) {
        return timer.read_reg(address);
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
    // Timer (0xFF04 - 0xFF07)
    if (address >= 0xFF04 && address <= 0xFF07) {
        timer.write_reg(address, value);
        return;
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

bool Bus::load_rom(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cerr << "Error:: cannot open rom " << filepath << "\n";
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size > 0x8000)
        size = 0x8000;

    if (file.read(reinterpret_cast<char*>(memory.data()), size)) {
        std::cout << "ROM loaded successfully (" << size << " bytes)\n";
        return true;
    }

    std::cerr << "Error reading ROM.\n";
    return false;
}