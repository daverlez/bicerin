#pragma once
#include <cstdint>
#include <array>
#include <stdexcept>
#include <iostream>

#include <mmu/mmu.h>

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

        enum Flag : uint8_t {
            Z = 1 << 7,
            N = 1 << 6,
            H = 1 << 5,
            C = 1 << 4
        };

        void set_flag(Flag flag, bool value) {
            if (value)
                f |= flag;
            else
                f &= ~flag;
        }

        bool get_flag(Flag flag) {
            return f & flag;
        }
    };

    Cpu(Mmu& mmu);
    ~Cpu() = default;

    void reset();
    uint8_t tick();

    [[nodiscard]] Registers get_registers() const {return registers_; };

private:
    Registers registers_;
    Mmu& mmu_;

    using InstructionHandler = uint8_t (Cpu::*)();
    std::array<InstructionHandler, 256> instructions_;

    uint8_t unimplemented_instruction();
    void build_instruction_table();

    uint8_t nop();

    template <uint8_t Cpu::Registers::*Reg>
    uint8_t inc_r8();
    uint8_t inc_hl();
};


