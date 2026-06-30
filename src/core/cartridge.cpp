#include <fstream>
#include <iostream>

#include "core/cartridge.h"
#include "core/mbc1.h"

class RomOnly : public Cartridge {
public:
    RomOnly(std::vector<uint8_t> data, uint8_t ram_size_code, const std::string& path, bool battery)
        : rom(std::move(data)), save_path(path), has_battery(battery)
    {
        size_t ram_bytes = 0;
        switch (ram_size_code) {
            case 0x02: ram_bytes = 8 * 1024; break;
            case 0x03: ram_bytes = 32 * 1024; break;
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

    ~RomOnly() override {
        if (has_battery && !ram.empty()) {
            std::ofstream file(save_path, std::ios::binary);
            if (file.is_open()) {
                file.write(reinterpret_cast<char*>(ram.data()), ram.size());
                std::cout << "Save completed successfully: " << save_path << "\n";
            } else
                std::cerr << "Error:: cannot create save file.\n";
        }
    }

    uint8_t read(uint16_t address) const override {
        if (address < 0x8000) {
            if (address < rom.size())
                return rom[address];
        } else if (address >= 0xA000 && address <= 0xBFFF) {
            size_t offset = address - 0xA000;
            if (offset < ram.size())
                return ram[offset];
        }
        return 0xFF;
    }

    void write(uint16_t address, uint8_t value) override {
        if (address >= 0xA000 && address <= 0xBFFF) {
            size_t offset = address - 0xA000;
            if (offset < ram.size())
                ram[offset] = value;
        }
    }

private:
    std::vector<uint8_t> rom;
    std::vector<uint8_t> ram;
    std::string save_path;
    bool has_battery;
};

std::unique_ptr<Cartridge> Cartridge::load(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error:: cannot oper ROM " << filepath << "\n";
        return nullptr;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> rom_data(size);
    if (!file.read(reinterpret_cast<char*>(rom_data.data()), size)) {
        std::cerr << "Error reading ROM.\n";
        return nullptr;
    }

    uint8_t cartridge_type = rom_data[0x0147];
    uint8_t ram_size_code = rom_data[0x0149];

    std::string save_path = filepath;
    size_t dot_pos = save_path.find_last_of('.');
    if (dot_pos != std::string::npos)
        save_path = save_path.substr(0, dot_pos) + ".sav";
    else
        save_path += ".sav";

    std::cout << "Cartridge loaded. Type: 0x" << std::hex << (int)cartridge_type << std::dec << "\n";

    switch (cartridge_type) {
        case 0x00: // ROM ONLY
            return std::make_unique<RomOnly>(std::move(rom_data), 0, save_path, false);
        case 0x08: // ROM + RAM
            return std::make_unique<RomOnly>(std::move(rom_data), ram_size_code, save_path, false);
        case 0x09: // ROM + RAM + BATTERY
            return std::make_unique<RomOnly>(std::move(rom_data), ram_size_code, save_path, true);

        case 0x01: // MBC1
            return std::make_unique<Mbc1>(std::move(rom_data), ram_size_code, save_path, false);
        case 0x02: // MBC1 + RAM
            return std::make_unique<Mbc1>(std::move(rom_data), ram_size_code, save_path, false);
        case 0x03: // MBC1 + RAM
            return std::make_unique<Mbc1>(std::move(rom_data), ram_size_code, save_path, true);

        default:
            std::cerr << "Cartridge type not supported yet: 0x" << std::hex << (int)cartridge_type << "\n";
            return std::make_unique<RomOnly>(std::move(rom_data), 0, save_path, false);
    }
}