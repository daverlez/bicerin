#include <fstream>
#include <iostream>

#include "core/mbc1.h"

Mbc1::Mbc1(std::vector<uint8_t> rom_data, uint8_t ram_size_code, const std::string& path, bool battery)
    : rom(std::move(rom_data)), save_path(path), has_battery(battery)
{
    size_t ram_bytes = 0;
    switch (ram_size_code) {
        case 0x02: ram_bytes = 8 * 1024;  break;
        case 0x03: ram_bytes = 32 * 1024; break;
        case 0x04: ram_bytes = 128 * 1024; break;
        case 0x05: ram_bytes = 64 * 1024; break;
    }
    ram.resize(ram_bytes, 0x00);

    if (has_battery && !ram.empty()) {
        std::ifstream file(save_path, std::ios::binary);
        if (file.is_open()) {
            file.read(reinterpret_cast<char*>(ram.data()), ram.size());
            std::cout << "Save loaded from: " << save_path << "\n";
        } else
            std::cout << "No save found: new game.\n";
    }
}

Mbc1::~Mbc1() {
    if (has_battery && !ram.empty()) {
        std::ofstream file(save_path, std::ios::binary);
        if (file.is_open()) {
            file.write(reinterpret_cast<char*>(ram.data()), ram.size());
            std::cout << "Save completed successfully: " << save_path << "\n";
        } else
            std::cerr << "Error:: cannot create save file.\n";
    }
}

uint8_t Mbc1::read(uint16_t address) const {
    if (address < 0x4000) {
        return rom[address];
    } 
    else if (address < 0x8000) {
        size_t offset = (address - 0x4000) + (rom_bank * 0x4000);

        if (offset < rom.size()) return rom[offset];
        return 0xFF;
    } 
    else if (address >= 0xA000 && address <= 0xBFFF) {
        if (!ram_enabled || ram.empty()) return 0xFF;
        
        size_t offset = (address - 0xA000) + (ram_bank * 0x2000);
        if (offset < ram.size()) return ram[offset];
        return 0xFF;
    }
    
    return 0xFF;
}

void Mbc1::write(uint16_t address, uint8_t value) {
    if (address < 0x2000) {
        // 0x0000-0x1FFF: RAM enable
        ram_enabled = ((value & 0x0F) == 0x0A);
    } 
    else if (address < 0x4000) {
        // 0x2000-0x3FFF: ROM bank (lower 5)
        uint8_t lower_5 = value & 0x1F;
        if (lower_5 == 0) lower_5 = 1; 
        
        rom_bank = (rom_bank & 0xE0) | lower_5;
    } 
    else if (address < 0x6000) {
        // 0x4000-0x5FFF: RAM bank / ROM bank (bits 5-6)
        if (banking_mode == 0) {
            // ROM mode
            rom_bank = (rom_bank & 0x1F) | ((value & 0x03) << 5);
        } else {
            // RAM mode
            ram_bank = value & 0x03;
        }
    } 
    else if (address < 0x8000) {
        // 0x6000-0x7FFF: Banking Mode Select
        banking_mode = value & 0x01;
    } 
    else if (address >= 0xA000 && address <= 0xBFFF) {
        if (!ram_enabled || ram.empty()) return;
        
        size_t offset = (address - 0xA000) + (ram_bank * 0x2000);
        if (offset < ram.size()) {
            ram[offset] = value;
        }
    }
}