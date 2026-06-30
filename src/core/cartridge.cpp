#include <fstream>
#include <iostream>

#include "core/cartridge.h"
#include "core/mbc1.h"

class RomOnly : public Cartridge {
public:
    RomOnly(std::vector<uint8_t> data) : rom(std::move(data)) {}

    uint8_t read(uint16_t address) const override {
        if (address < 0x8000)
            if (address < rom.size())
                return rom[address];

        return 0xFF;
    }

    void write([[maybe_unused]] uint16_t address, [[maybe_unused]] uint8_t value) override { }

private:
    std::vector<uint8_t> rom;
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

    std::cout << "Cartridge loaded. Type: 0x" << std::hex << (int)cartridge_type << std::dec << "\n";

    switch (cartridge_type) {
        case 0x00: // ROM ONLY
        case 0x08: // ROM + RAM
        case 0x09: // ROM + RAM + BATTERY
            return std::make_unique<RomOnly>(std::move(rom_data));

        case 0x01: // MBC1
        case 0x02: // MBC1 + RAM
        case 0x03: // MBC1 + RAM + BATTERY
            return std::make_unique<Mbc1>(std::move(rom_data), ram_size_code);

        default:
            std::cerr << "MBC not supported yet: 0x" << std::hex << (int)cartridge_type << "\n";
            return std::make_unique<RomOnly>(std::move(rom_data));
    }
}