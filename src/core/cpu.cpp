#include "core/cpu.h"

#include <iostream>

constexpr std::array<uint8_t, 256> INSTRUCTION_CYCLES = {
    1, 3, 2, 2, 1, 1, 2, 1, 5, 2, 2, 2, 1, 1, 2, 1, // 0x00 - 0x0F
    1, 3, 2, 2, 1, 1, 2, 1, 3, 2, 2, 2, 1, 1, 2, 1, // 0x10 - 0x1F (0x10 = STOP: 1)
    2, 3, 2, 2, 1, 1, 2, 1, 2, 2, 2, 2, 1, 1, 2, 1, // 0x20 - 0x2F (0x20 = JR NZ: base 2)
    2, 3, 2, 2, 3, 3, 3, 1, 2, 2, 2, 2, 1, 1, 2, 1, // 0x30 - 0x3F
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // 0x40 - 0x4F
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // 0x50 - 0x5F
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // 0x60 - 0x6F
    2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, // 0x70 - 0x7F (0x76 = HALT: 1)
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // 0x80 - 0x8F
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // 0x90 - 0x9F
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // 0xA0 - 0xAF
    1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, // 0xB0 - 0xBF
    2, 3, 3, 4, 3, 4, 2, 4, 2, 4, 3, 0, 3, 6, 2, 4, // 0xC0 - 0xCF (0xC0 = RET NZ: base 2)
    2, 3, 3, 0, 3, 4, 2, 4, 2, 4, 3, 0, 3, 0, 2, 4, // 0xD0 - 0xDF
    3, 3, 2, 0, 0, 4, 2, 4, 4, 1, 4, 0, 0, 0, 2, 4, // 0xE0 - 0xEF
    3, 3, 2, 1, 0, 4, 2, 4, 3, 2, 4, 1, 0, 0, 2, 4  // 0xF0 - 0xFF
};

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

uint8_t Cpu::step(Bus& bus) {
    uint8_t interrupt_cycles = handle_interrupts(bus);
    if (interrupt_cycles > 0) return interrupt_cycles;

    if (halted) return 1;

    uint8_t opcode = fetch8(bus);
    uint8_t base_cycles = INSTRUCTION_CYCLES[opcode];

    uint8_t penalty_cycles = execute(opcode, bus);
    return base_cycles + penalty_cycles;
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

uint8_t Cpu::execute(uint8_t opcode, Bus& bus) {
    uint8_t x = (opcode >> 6) & 0x03;
    uint8_t y = (opcode >> 3) & 0x07;
    uint8_t z = opcode & 0x07;

    /***********
     * Block 0 *
     ***********/
    if (x == 0) {
        if (z == 0) {
            if (y == 0) {
                // NOP
                return 0;
            } else if (y == 1) {
                // LD [imm16], SP
                uint16_t addr = fetch16(bus);
                bus.write(addr, static_cast<uint8_t>(sp & 0xFF));
                bus.write(addr + 1, static_cast<uint8_t>((sp >> 8) & 0xFF));
                return 0;
            } else if (y == 2) {
                // STOP
                fetch8(bus);
                std::cout << "STOP instruction executed (Not yet fully implemented)\n";
                return 0;
            } else if (y == 3) {
                // JR imm8
                int8_t signed_offset = static_cast<int8_t>(fetch8(bus));
                pc += signed_offset;
                return 0;
            } else if (y >= 4) {
                // JR cond, imm8
                uint8_t raw_offset = fetch8(bus);
                int8_t signed_offset = static_cast<int8_t>(raw_offset);

                if (check_cond(y - 4)) {
                    pc += signed_offset;
                    return 1;
                }
                return 0;
            }
        }

        if (z == 1) {
            if ((y % 2) == 0) {
                // LD r16, imm16
                uint8_t reg_index = y / 2;
                uint16_t value = fetch16(bus);
                set_r16(reg_index, value);
                return 0;
            } else {
                // ADD HL, r16
                uint8_t reg_index = y / 2;
                uint16_t target = get_r16(reg_index);
                uint16_t current_hl = get_hl();
                uint32_t result = static_cast<uint32_t>(current_hl) + target;

                set_flag_n(false);
                set_flag_h((((current_hl & 0x0FFF) + (target & 0x0FFF)) & 0x1000) != 0);
                set_flag_c(result > 0xFFFF);

                set_hl(static_cast<uint16_t>(result));
                return 0;
            }
        }

        if (z == 2) {
            uint8_t reg_index = y / 2;
            uint16_t addr;

            if (reg_index == 0) {
                addr = get_bc();
            } else if (reg_index == 1) {
                addr = get_de();
            } else if (reg_index == 2) {
                addr = get_hl();
                set_hl(addr + 1); // HL+
            } else {
                addr = get_hl();
                set_hl(addr - 1); // HL-
            }

            if ((y % 2) == 0) {
                bus.write(addr, a); // LD [r16mem], A
            } else {
                a = bus.read(addr); // LD A, [r16mem]
            }
            return 0;
        }

        if (z == 3) {
            uint8_t reg_index = y / 2;
            uint16_t val = get_r16(reg_index);

            if ((y % 2) == 0)
                set_r16(reg_index, val + 1); // INC r16
            else
                set_r16(reg_index, val - 1); // DEC r16

            return 0;
        }

        if (z == 4) {
            // INC r8
            uint8_t val = get_reg8(y, bus);
            uint8_t result = val + 1;
            set_reg8(y, result, bus);

            set_flag_z(result == 0);
            set_flag_n(false);
            set_flag_h((val & 0x0F) == 0x0F);

            return 0;
        }

        if (z == 5) {
            // DEC r8
            uint8_t val = get_reg8(y, bus);
            uint8_t result = val - 1;
            set_reg8(y, result, bus);

            set_flag_z(result == 0);
            set_flag_n(true);
            set_flag_h((val & 0x0F) == 0x00);

            return 0;
        }

        if (z == 6) {
            // LD r8, imm8
            uint8_t value = fetch8(bus);
            set_reg8(y, value, bus);
            return 0;
        }

        if (z == 7) {
            uint8_t carry_out;
            uint8_t old_c = get_flag_c() ? 1 : 0;

            switch (y) {
                case 0:
                    // RLCA
                    carry_out = (a >> 7) & 1;
                    a = (a << 1) | carry_out;
                    set_flag_z(false);
                    set_flag_n(false);
                    set_flag_h(false);
                    set_flag_c(carry_out != 0);
                    break;
                case 1:
                    // RRCA
                    carry_out = a & 1;
                    a = (a >> 1) | (carry_out << 7);
                    set_flag_z(false);
                    set_flag_n(false);
                    set_flag_h(false);
                    set_flag_c(carry_out != 0);
                    break;
                case 2:
                    // RLA
                    carry_out = (a >> 7) & 1;
                    a = (a << 1) | old_c;
                    set_flag_z(false);
                    set_flag_n(false);
                    set_flag_h(false);
                    set_flag_c(carry_out != 0);
                    break;
                case 3:
                    // RRA
                    carry_out = a & 1;
                    a = (a >> 1) | (old_c << 7);
                    set_flag_z(false);
                    set_flag_n(false);
                    set_flag_h(false);
                    set_flag_c(carry_out != 0);
                    break;
                case 4: {
                    // DAA (Decimal Adjust Accumulator)
                    uint8_t adjust = 0;
                    bool carry = false;

                    if (get_flag_h() || (!get_flag_n() && (a & 0x0F) > 0x09))
                        adjust |= 0x06;

                    if (get_flag_c() || (!get_flag_n() && a > 0x99)) {
                        adjust |= 0x60;
                        carry = true;
                    }

                    a += get_flag_n() ? -adjust : adjust;

                    set_flag_z(a == 0);
                    set_flag_h(false);
                    set_flag_c(carry || get_flag_c());
                    break;
                }
                case 5:
                    // CPL (Complement A)
                    a = ~a;
                    set_flag_n(true);
                    set_flag_h(true);
                    break;
                case 6:
                    // SCF (Set Carry Flag)
                    set_flag_n(false);
                    set_flag_h(false);
                    set_flag_c(true);
                    break;
                case 7:
                    // CCF (Complement Carry Flag)
                    set_flag_n(false);
                    set_flag_h(false);
                    set_flag_c(!get_flag_c());
                    break;
            }
            return 0;
        }
    }

    /***********
     * Block 1 *
     ***********/
    if (x == 1) {
        if (y == 6 && z == 6) {
            // HALT
            halted = true;
            return 0;
        }

        // LD r8, r8
        uint8_t value = get_reg8(z, bus);
        set_reg8(y, value, bus);
        return 0;
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
        return 0;
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
            return 0;
        }

        if (z == 0) {
            if (y < 4) {
                // RET cond
                if (check_cond(y)) {
                    pc = pop(bus);
                    return 3;
                }
                return 0;
            } else if (y == 4) {
                // LDH [imm8], A
                uint8_t offset = fetch8(bus);
                bus.write(0xFF00 + offset, a);
                return 0;
            } else if (y == 6) {
                // LDH A, [imm8]
                uint8_t offset = fetch8(bus);
                a = bus.read(0xFF00 + offset);
                return 0;
            } else if (y == 5 || y == 7) {
                uint8_t raw_offset = fetch8(bus);
                int8_t signed_offset = static_cast<int8_t>(raw_offset);

                uint16_t result = sp + signed_offset;

                set_flag_z(false);
                set_flag_n(false);
                set_flag_h(((sp & 0x0F) + (raw_offset & 0x0F)) > 0x0F);
                set_flag_c(((sp & 0xFF) + (raw_offset & 0xFF)) > 0xFF);

                if (y == 5) {
                    // ADD SP, imm8
                    sp = result;
                } else {
                    // LD HL, SP + imm8
                    set_hl(result);
                }
                return 0;
            }
        }

        if (z == 1) {
            if ((y % 2) == 0) {
                // Stack POP instruction (bit 3 == 0)
                uint8_t reg_index = y / 2;
                uint16_t value = pop(bus);
                set_r16stk(reg_index, value);
                return 0;
            } else {
                // Others (bit 3 == 1)
                switch (y) {
                    case 1: pc = pop(bus); break;               // RET
                    case 3: pc = pop(bus); ime = true; break;   // RETI
                    case 5: pc = get_hl(); break;                  // JP HL
                    case 7: sp = get_hl(); break;                  // LD SP, HL
                }
                return 0;
            }
        }

        if (z == 2) {
            if (y < 4) {
                // JP cond, imm16
                uint16_t addr = fetch16(bus);
                if (check_cond(y)) {
                    pc = addr;
                    return 1;
                }
                return 0;
            } else if (y == 4) {
                // LDH [c], A
                bus.write(0xFF00 + c, a);
                return 0;
            } else if (y == 5) {
                // LD [imm16], A
                uint16_t addr = fetch16(bus);
                bus.write(addr, a);
                return 0;
            } else if (y == 6) {
                // LDH A, [c]
                a = bus.read(0xFF00 + c);
                return 0;
            } else if (y == 7) {
                // LD A, [imm16]
                uint16_t addr = fetch16(bus);
                a = bus.read(addr);
                return 0;
            }
        }

        if (z == 3) {
            if (y == 0) {
                // JP imm16
                pc = fetch16(bus);
                return 0;
            } else if (y == 1) {
                // CB
                uint8_t cb_opcode = fetch8(bus);
                return execute_cb(cb_opcode, bus);
            } else if (y == 6) {
                // DI
                ime = false;
                return 0;
            } else if (y == 7) {
                // EI
                ime = true;
                return 0;
            }
        }

        // CALL cond, imm16
        if (z == 4) {
            if (y < 4) {
                uint16_t addr = fetch16(bus);
                if (check_cond(y)) {
                    push(pc, bus);
                    pc = addr;
                    return 3;
                }
                return 0;
            }
        }

        if (z == 5) {
            if ((y % 2) == 0) {
                // Stack PUSH instruction (bit 3 == 0)
                uint8_t reg_index = y / 2;
                uint16_t value = get_r16stk(reg_index);
                push(value, bus);
                return 0;
            } else {
                if (y == 1) {
                    // CALL imm16 (bit 3 == 1)
                    uint16_t addr = fetch16(bus);
                    push(pc, bus);
                    pc = addr;
                    return 0;
                }
            }
        }

        if (z == 7) {
            // RST tgt3
            push(pc, bus);
            pc = y * 8;
            return 0;
        }
    }

    std::cerr << "Error:: Opcode not implemented:: 0x"
              << std::hex << (int)opcode
              << " at PC address:: 0x"
              << (pc - 1) << "\n";

    return 0;
}

uint8_t Cpu::execute_cb(uint8_t cb_opcode, Bus& bus) {
    uint8_t cb_x = (cb_opcode >> 6) & 0x03;
    uint8_t cb_y = (cb_opcode >> 3) & 0x07;
    uint8_t cb_z = cb_opcode & 0x07;

    uint8_t target_reg = get_reg8(cb_z, bus);

    if (cb_x == 0) {
        uint8_t carry_out = 0;
        uint8_t result = target_reg;

        switch (cb_y) {
            case 0:
                // RLC (Rotate Left Circular)
                carry_out = (target_reg >> 7) & 1;
                result = (target_reg << 1) | carry_out;
                break;
            case 1:
                // RRC (Rotate Right Circular)
                carry_out = target_reg & 1;
                result = (target_reg >> 1) | (carry_out << 7);
                break;
            case 2:
                // RL (Rotate Left through Carry)
                carry_out = (target_reg >> 7) & 1;
                result = (target_reg << 1) | (get_flag_c() ? 1 : 0);
                break;
            case 3:
                // RR (Rotate Right through Carry)
                carry_out = target_reg & 1;
                result = (target_reg >> 1) | (get_flag_c() ? 0x80 : 0);
                break;
            case 4:
                // SLA (Shift Left Arithmetic)
                carry_out = (target_reg >> 7) & 1;
                result = target_reg << 1;
                break;
            case 5:
                // SRA (Shift Right Arithmetic)
                carry_out = target_reg & 1;
                result = (target_reg >> 1) | (target_reg & 0x80);
                break;
            case 6:
                // SWAP
                carry_out = 0;
                result = ((target_reg & 0x0F) << 4) | ((target_reg & 0xF0) >> 4);
                break;
            case 7:
                // SRL (Shift Right Logical)
                carry_out = target_reg & 1;
                result = target_reg >> 1;
                break;
        }

        set_reg8(cb_z, result, bus);

        set_flag_z(result == 0);
        set_flag_n(false);
        set_flag_h(false);
        set_flag_c(carry_out != 0);
    }

    if (cb_x == 1) {
        // BIT b3, r8
        bool bit_is_zero = (target_reg & (1 << cb_y)) == 0;

        set_flag_z(bit_is_zero);
        set_flag_n(false);
        set_flag_h(true);
    }

    if (cb_x == 2) {
        // RES b3, r8
        target_reg &= ~(1 << cb_y);
        set_reg8(cb_z, target_reg, bus);
    }

    if (cb_x == 3) {
        // SET b3, r8
        target_reg |= (1 << cb_y);
        set_reg8(cb_z, target_reg, bus);
    }

    if (cb_x == 1)
        return (cb_z == 6) ? 3 : 2; // BIT
    else
        return (cb_z == 6) ? 4 : 2; // All the others
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

void Cpu::set_r16stk(uint8_t index, uint16_t value) {
    switch (index) {
        case 0: set_bc(value); break;
        case 1: set_de(value); break;
        case 2: set_hl(value); break;
        case 3: set_af(value); break;
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

void Cpu::set_r16(uint8_t index, uint16_t value) {
    switch (index) {
        case 0: set_bc(value); break;
        case 1: set_de(value); break;
        case 2: set_hl(value); break;
        case 3: sp = value;    break;
    }
}

uint16_t Cpu::get_r16(uint8_t index) const {
    switch (index) {
        case 0: return get_bc();
        case 1: return get_de();
        case 2: return get_hl();
        case 3: return sp;
        default: return 0;
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

uint8_t Cpu::handle_interrupts(Bus& bus) {
    uint8_t ie = bus.read(0xFFFF);
    uint8_t if_reg = bus.read(0xFF0F);

    uint8_t pending = ie & if_reg & 0x1F;

    if (pending > 0) {
        halted = false;

        if (ime) {
            ime = false;

            for (uint8_t i = 0; i < 5; i++) {
                if (pending & (1 << i)) {
                    bus.write(0xFF0F, if_reg & ~(1 << i));
                    push(pc, bus);

                    // Bit 0: 0x40 (VBlank)
                    // Bit 1: 0x48 (LCD STAT)
                    // Bit 2: 0x50 (Timer)
                    // Bit 3: 0x58 (Serial)
                    // Bit 4: 0x60 (Joypad)
                    pc = 0x0040 + (i * 8);

                    return 5;
                }
            }
        }
    }

    return 0;
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