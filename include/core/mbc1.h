#pragma once
#include "core/cartridge.h"
#include <vector>

class Mbc1 : public Cartridge {
public:
    Mbc1(std::vector<uint8_t> rom_data, uint8_t ram_size_code, const std::string& save_path, bool has_battery);
    ~Mbc1() override;

    uint8_t read(uint16_t address) const override;
    void write(uint16_t address, uint8_t value) override;

private:
    std::vector<uint8_t> rom;
    std::vector<uint8_t> ram;

    std::string save_path;
    bool has_battery;

    bool ram_enabled{false};
    uint8_t banking_mode{0}; // 0 = ROM Banking Mode, 1 = RAM Banking Mode

    uint8_t rom_bank{1}; 
    uint8_t ram_bank{0};
};