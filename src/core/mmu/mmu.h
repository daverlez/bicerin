#pragma once
#include <array>
#include <cstdint>

class Mmu {
public:
    Mmu();
    ~Mmu() = default;

    void reset();

    [[nodiscard]] uint8_t read(uint16_t address) const;
    void write(uint16_t address, uint8_t data);

private:
    std::array<uint8_t, 0x10000> memory_;
};
