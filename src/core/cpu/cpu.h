#pragma once
#include <cstdint>

class MMU;

class Cpu {
public:
    struct Registers {
        uint8_t a{0x00};
        uint8_t b{0x00};
        uint8_t c{0x00};
        uint8_t d{0x00};
        uint8_t e{0x00};
        uint8_t f{0x00};
        uint8_t h{0x00};
        uint8_t l{0x00};
        uint16_t sp{0x0000};
        uint16_t pc{0x0100};
    };

    Cpu();
    ~Cpu() = default;

    void reset();
    auto tick() -> uint8_t;

    [[nodiscard]] Registers get_registers() const {return registers_; };

private:
    Registers registers_;
    //MMU& mmu_;
};


