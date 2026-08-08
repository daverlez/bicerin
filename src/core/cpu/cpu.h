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

        [[nodiscard]] uint16_t get_af() const { return (a << 8) | f; }
        [[nodiscard]] uint16_t get_bc() const { return (b << 8) | c; }
        [[nodiscard]] uint16_t get_de() const { return (d << 8) | e; }
        [[nodiscard]] uint16_t get_hl() const { return (h << 8) | l; }

        void set_af(uint16_t val) { a = (val & 0xFF00) >> 8; f = (val & 0x00FF); }
        void set_bc(uint16_t val) { b = (val & 0xFF00) >> 8; c = (val & 0x00FF); }
        void set_de(uint16_t val) { d = (val & 0xFF00) >> 8; e = (val & 0x00FF); }
        void set_hl(uint16_t val) { h = (val & 0xFF00) >> 8; l = (val & 0x00FF); }
    };

    Cpu();
    ~Cpu() = default;

    void reset();
    [[nodiscard]] uint8_t tick();

    [[nodiscard]] Registers get_registers() const {return registers_; };

private:
    Registers registers_;
    //MMU& mmu_;
};


