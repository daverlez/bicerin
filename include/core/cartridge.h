#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <string>

class Cartridge {
public:
    virtual ~Cartridge() = default;

    /// Reads a byte from the ROM/RAM of the cartridge.
    virtual uint8_t read(uint16_t address) const = 0;

    /// Writes a byte: used for RAM saves or for MBC commands.
    virtual void write(uint16_t address, uint8_t value) = 0;

    /// Factory: loads the file, reads the header and returns the MBC.
    static std::unique_ptr<Cartridge> load(const std::string& filepath);
};