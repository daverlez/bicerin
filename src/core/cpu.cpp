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
    uint8_t opcode = fetch8(bus);
    execute(opcode, bus);
}

uint8_t Cpu::fetch8(Bus& bus) {
    uint8_t opcode = bus.read(pc);
    pc++;
    return opcode;
}

uint16_t Cpu::fetch16(Bus& bus) {
    uint8_t lo = fetch8(bus);
    uint8_t hi = fetch8(bus);
    return (static_cast<uint16_t>(hi) << 8) | lo;
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
            // LD r8, imm8
            uint8_t value = fetch8(bus);
            set_reg8(y, value, bus);
            return;
        }
    }

    /***********
     * Block 1 *
     ***********/
    if (x == 1) {
        if (y == 6 && z == 6) {
            // HALT
            std::cout << "HALT instruction executed (Not yet implemented)\n";
            return;
        }

        // LD r8, r8
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
            uint8_t operand = fetch8(bus);

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

        if (z == 0) {
            if (y < 4) {
                // RET cond
                if (check_cond(y))
                    pc = pop(bus);
            } else if (y == 4) {
                // LDH [imm8], A
                uint8_t offset = fetch8(bus);
                bus.write(0xFF00 + offset, a);
            } else if (y == 6) {
                // LDH A, [imm8]
                uint8_t offset = fetch8(bus);
                a = bus.read(0xFF00 + offset);
            }
            return;
        }

        if (z == 1) {
            if ((y % 2) == 0) {
                // Stack POP instruction (bit 3 == 0)
                uint8_t reg_index = y / 2;
                uint16_t value = pop(bus);
                set_r16stk(reg_index, value);
            } else {
                // Others (bit 3 == 1)
                switch (y) {
                    case 1: pc = pop(bus); break;   // RET
                    case 3: pc = pop(bus); break;   // TODO: enable interrupts (RETI)
                    case 5: pc = get_hl(); break;      // JP HL
                    case 7: sp = get_hl(); break;      // LD SP, HL
                }
            }
            return;
        }

        if (z == 2) {
            if (y < 4) {
                // JP cond, imm16
                uint16_t addr = fetch16(bus);
                if (check_cond(y))
                    pc = addr;
            } else if (y == 4) {
                // LDH [c], A
                bus.write(0xFF00 + c, a);
            } else if (y == 5) {
                // LD [imm16], A
                uint16_t addr = fetch16(bus);
                bus.write(addr, a);
            } else if (y == 6) {
                // LDH A, [c]
                a = bus.read(0xFF00 + c);
            } else if (y == 7) {
                // LD A, [imm16]
                uint16_t addr = fetch16(bus);
                a = bus.read(addr);
            }
            return;
        }

        if (z == 3) {
            // JP imm16
            if (y == 0) {
                pc = fetch16(bus); // JP imm16
                return;
            }

            // TODO: Prefix CB, DI, EI
        }

        // CALL cond, imm16
        if (z == 4) {
            if (y < 4) {
                uint16_t addr = fetch16(bus);
                if (check_cond(y)) {
                    push(pc, bus);
                    pc = addr;
                }
                return;
            }
        }

        if (z == 5) {
            if ((y % 2) == 0) {
                // Stack PUSH instruction (bit 3 == 0)
                uint8_t reg_index = y / 2;
                uint16_t value = get_r16stk(reg_index);
                push(value, bus);
            } else {
                // CALL imm16 (bit 3 == 1)
                if (y == 1) {
                    uint16_t addr = fetch16(bus);
                    push(pc, bus);
                    pc = addr;
                }
            }
            return;
        }

        if (z == 7) {
            // RST tgt3
            push(pc, bus);
            pc = y * 8;
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

uint16_t Cpu::get_r16stk(uint8_t index) const {
    switch (index) {
        case 0: return get_bc();
        case 1: return get_de();
        case 2: return get_hl();
        case 3: return get_af();
        default: return 0;
    }
}

void Cpu::set_r16stk(uint8_t index, uint16_t value) {
    switch (index) {
        case 0: set_bc(value); break;
        case 1: set_de(value); break;
        case 2: set_hl(value); break;
        case 3: set_af(value); break;
    }
}

void Cpu::push(uint16_t value, Bus& bus) {
    sp--;
    bus.write(sp, static_cast<uint8_t>((value >> 8) & 0xFF));
    sp--;
    bus.write(sp, static_cast<uint8_t>(value & 0xFF));
}

uint16_t Cpu::pop(Bus& bus) {
    uint8_t lo = bus.read(sp);
    sp++;
    uint8_t hi = bus.read(sp);
    sp++;
    return (static_cast<uint16_t>(hi) << 8) | lo;
}

bool Cpu::check_cond(uint8_t cond) const {
    switch (cond) {
        case 0: return !get_flag_z(); // NZ
        case 1: return get_flag_z();  // Z
        case 2: return !get_flag_c(); // NC
        case 3: return get_flag_c();  // C
        default: return false;
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