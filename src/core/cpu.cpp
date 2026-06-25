#include "core/cpu.h"

#include <iostream>

Cpu::Cpu() {
    reset();
}

void Cpu::reset() {
    a  = 0x00;
    f  = 0x00;
    b  = 0x00;
    c  = 0x00;
    d  = 0x00;
    e  = 0x00;
    h  = 0x00;
    l  = 0x00;
    sp = 0x0000;
    pc = 0x0100;
}

void Cpu::step(Bus& bus) {
    uint8_t opcode = fetch(bus);
    execute(opcode, bus);
}

uint8_t Cpu::fetch(Bus& bus) {
    uint8_t opcode = bus.read(pc);
    pc++;
    return opcode;
}

void Cpu::execute(uint8_t opcode, Bus& bus) {
    uint8_t x = (opcode >> 6) & 0x03;
    uint8_t y = (opcode >> 3) & 0x07;
    uint8_t z = opcode & 0x07;

    /***********
     * Block 0 *
     ***********/
    if (x == 0) {
        if (z == 6) {
            /*
             * LD r8, imm8
             * Loads the next byte in the address specified by 'y'.
             */
            uint8_t value = fetch(bus);
            set_reg8(y, value, bus);
            return;
        }
    }

    /***********
     * Block 1 *
     ***********/
    if (x == 1) {
        if (y == 6 && z == 6) {
            /*
             * HALT
             * Pauses the CPU until the next interrupt.
             */
            std::cout << "HALT instruction executed (Not yet implemented)\n";
            return;
        }

        /*
         * LD r8, r8
         * Copies the value from register 'z' to register 'y'.
         */
        uint8_t value = get_reg8(z, bus);
        set_reg8(y, value, bus);
        return;
    }

    /***********
     * Block 2 *
     ***********/
    if (x == 2) {
        uint8_t operand = get_reg8(z, bus);

        switch (y) {
            case 0: add_a(operand); break; // ADD
            case 1: adc_a(operand); break; // ADC
            case 2: sub_a(operand); break; // SUB
            case 3: sbc_a(operand); break; // SBC
            case 4: and_a(operand); break; // AND
            case 5: xor_a(operand); break; // XOR
            case 6: or_a(operand);  break; // OR
            case 7: cp_a(operand);  break; // CP
        }
        return;
    }

    /***********
     * Block 3 *
     ***********/
    if (x == 3) {
        // Arithmetic/Logical operations with imm8
        if (z == 6) {
            uint8_t operand = fetch(bus);

            switch (y) {
                case 0: add_a(operand); break; // ADD A, imm8
                case 1: adc_a(operand); break; // ADC A, imm8
                case 2: sub_a(operand); break; // SUB A, imm8
                case 3: sbc_a(operand); break; // SBC A, imm8
                case 4: and_a(operand); break; // AND A, imm8
                case 5: xor_a(operand); break; // XOR A, imm8
                case 6: or_a(operand);  break; // OR A, imm8
                case 7: cp_a(operand);  break; // CP A, imm8
            }
            return;
        }
    }

    switch (opcode) {
        case 0x00:
            break;

        default:
            std::cerr << "Error:: Opcode not implemented:: 0x"
                      << std::hex << (int)opcode
                      << " at PC address:: 0x"
                      << (pc - 1) << "\n";
            break;
    }
}

void Cpu::set_reg8(uint8_t index, uint8_t value, Bus& bus) {
    switch (index) {
        case 0: b = value; break;
        case 1: c = value; break;
        case 2: d = value; break;
        case 3: e = value; break;
        case 4: h = value; break;
        case 5: l = value; break;
        case 6: bus.write(get_hl(), value); break;
        case 7: a = value; break;
    }
}

uint8_t Cpu::get_reg8(uint8_t index, Bus& bus) const {
    switch (index) {
        case 0: return b;
        case 1: return c;
        case 2: return d;
        case 3: return e;
        case 4: return h;
        case 5: return l;
        case 6: return bus.read(get_hl());
        case 7: return a;
        default: return 0;
    }
}

void Cpu::add_a(uint8_t value) {
    uint16_t result = static_cast<uint16_t>(a) + value;

    set_flag_z((result & 0xFF) == 0);
    set_flag_n(false);
    set_flag_h((((a & 0x0F) + (value & 0x0F)) & 0x10) != 0);
    set_flag_c(result > 0xFF);

    a = static_cast<uint8_t>(result);
}

void Cpu::adc_a(uint8_t value) {
    uint8_t carry = get_flag_c() ? 1 : 0;
    uint16_t result = static_cast<uint16_t>(a) + value + carry;

    set_flag_z((result & 0xFF) == 0);
    set_flag_n(false);
    set_flag_h((((a & 0x0F) + (value & 0x0F) + carry) & 0x10) != 0);
    set_flag_c(result > 0xFF);

    a = static_cast<uint8_t>(result);
}

void Cpu::sub_a(uint8_t value) {
    int result = static_cast<int>(a) - value;

    set_flag_z((result & 0xFF) == 0);
    set_flag_n(true);
    set_flag_h((a & 0x0F) < (value & 0x0F));
    set_flag_c(a < value);

    a = static_cast<uint8_t>(result);
}

void Cpu::sbc_a(uint8_t value) {
    uint8_t carry = get_flag_c() ? 1 : 0;
    int result = static_cast<int>(a) - value - carry;

    set_flag_z((result & 0xFF) == 0);
    set_flag_n(true);
    set_flag_h((a & 0x0F) < ((value & 0x0F) + carry));

    uint16_t res16 = static_cast<uint16_t>(a) - static_cast<uint16_t>(value) - static_cast<uint16_t>(carry);
    set_flag_c(res16 > 0xFF);

    a = static_cast<uint8_t>(result);
}

void Cpu::and_a(uint8_t value) {
    a &= value;
    set_flag_z(a == 0);
    set_flag_n(false);
    set_flag_h(true);
    set_flag_c(false);
}

void Cpu::xor_a(uint8_t value) {
    a ^= value;
    set_flag_z(a == 0);
    set_flag_n(false);
    set_flag_h(false);
    set_flag_c(false);
}

void Cpu::or_a(uint8_t value) {
    a |= value;
    set_flag_z(a == 0);
    set_flag_n(false);
    set_flag_h(false);
    set_flag_c(false);
}

void Cpu::cp_a(uint8_t value) {
    int result = static_cast<int>(a) - value;

    set_flag_z((result & 0xFF) == 0);
    set_flag_n(true);
    set_flag_h((a & 0x0F) < (value & 0x0F));
    set_flag_c(a < value);
}