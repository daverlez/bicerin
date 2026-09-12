#include "cpu.h"

Cpu::Cpu(Mmu& mmu) : mmu_(mmu) {
    build_instruction_table();
    reset();
}

void Cpu::reset() {
    registers_.a = 0x00;
    registers_.b = 0x00;
    registers_.c = 0x00;
    registers_.d = 0x00;
    registers_.e = 0x00;
    registers_.f = 0x00;
    registers_.h = 0x00;
    registers_.l = 0x00;
    registers_.sp = 0x0000;
    registers_.pc = 0x0100;
}

uint8_t Cpu::tick() {
    uint8_t opcode = mmu_.read(registers_.pc);
    registers_.pc++;

    return (this->*instructions_[opcode])();
}

uint8_t Cpu::unimplemented_instruction() {
    uint8_t opcode = mmu_.read(registers_.pc - 1);
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "Unimplemented opcode: 0x%02X at PC: 0x%04X", opcode, registers_.pc - 1);

    std::cerr << buffer << std::endl;
    throw std::runtime_error(buffer);
}

uint8_t Cpu::nop() {
    return 1;
}

template <void (Cpu::Registers::*Setter)(uint16_t)>
uint8_t Cpu::ld_r16_imm16() {
    uint8_t lsb = mmu_.read(registers_.pc); registers_.pc++;
    uint8_t msb = mmu_.read(registers_.pc); registers_.pc++;
    uint16_t imm16 = (msb << 8) | lsb;

    (registers_.*Setter)(imm16);
    return 3;
}

template <uint16_t (Cpu::Registers::*Getter)() const>
uint8_t Cpu::ld_r16mem_a() {
    uint16_t address = (registers_.*Getter)();
    mmu_.write(address, registers_.a);
    return 2;
}

uint8_t Cpu::ld_hli_a() {
    uint16_t hl = registers_.get_hl();
    mmu_.write(hl, registers_.a);
    registers_.set_hl(hl + 1);
    return 2;
}

uint8_t Cpu::ld_hld_a() {
    uint16_t hl = registers_.get_hl();
    mmu_.write(hl, registers_.a);
    registers_.set_hl(hl - 1);
    return 2;
}

template <uint16_t (Cpu::Registers::*Getter)() const>
uint8_t Cpu::ld_a_r16mem() {
    uint16_t address = (registers_.*Getter)();
    registers_.a = mmu_.read(address);
    return 2;
}

uint8_t Cpu::ld_a_hli() {
    uint16_t hl = registers_.get_hl();
    registers_.a = mmu_.read(hl);
    registers_.set_hl(hl + 1);
    return 2;
}

uint8_t Cpu::ld_a_hld() {
    uint16_t hl = registers_.get_hl();
    registers_.a = mmu_.read(hl);
    registers_.set_hl(hl - 1);
    return 2;
}

uint8_t Cpu::ld_imm16_sp() {
    uint8_t lsb = mmu_.read(registers_.pc); registers_.pc++;
    uint8_t msb = mmu_.read(registers_.pc); registers_.pc++;
    uint16_t imm16 = (msb << 8) | lsb;

    mmu_.write(imm16, registers_.sp & 0x00FF);
    mmu_.write(imm16 + 1, registers_.sp >> 8);
    return 5;
}

template <uint16_t (Cpu::Registers::*Getter)() const, void (Cpu::Registers::*Setter)(uint16_t)>
uint8_t Cpu::inc_r16() {
    uint16_t val = (registers_.*Getter)();
    (registers_.*Setter)(val + 1);
    return 2;
}

template <uint16_t (Cpu::Registers::*Getter)() const, void (Cpu::Registers::*Setter)(uint16_t)>
uint8_t Cpu::dec_r16() {
    uint16_t val = (registers_.*Getter)();
    (registers_.*Setter)(val - 1);
    return 2;
}

template <uint16_t (Cpu::Registers::*Getter)() const>
uint8_t Cpu::add_hl_r16() {
    uint16_t val = (registers_.*Getter)();
    uint16_t hl = registers_.get_hl();
    uint32_t res = static_cast<uint32_t>(hl) + val;
    registers_.set_hl(static_cast<uint16_t>(res));

    registers_.set_flag(Registers::Flag::N, false);
    registers_.set_flag(Registers::Flag::H, (((hl & 0x0FFF) + (val & 0xFFF)) & 0x1000) != 0);
    registers_.set_flag(Registers::Flag::C, res > 0x0000FFFF);

    return 2;
}

template <uint8_t Cpu::Registers::*Reg>
uint8_t Cpu::inc_r8() {
    bool half_carry = (registers_.*Reg & 0x0F) == 0x0F;
    registers_.*Reg += 1;

    registers_.set_flag(Registers::Flag::Z, registers_.*Reg == 0);
    registers_.set_flag(Registers::Flag::N, false);
    registers_.set_flag(Registers::Flag::H, half_carry);

    return 1;
}

uint8_t Cpu::inc_hl() {
    uint16_t address = registers_.get_hl();
    uint8_t val = mmu_.read(address);
    bool half_carry = (val & 0x0F) == 0x0F;
    val += 1;

    registers_.set_flag(Registers::Flag::Z, val == 0);
    registers_.set_flag(Registers::Flag::N, false);
    registers_.set_flag(Registers::Flag::H, half_carry);

    mmu_.write(address, val);

    return 3;
}

template <uint8_t Cpu::Registers::*Reg>
uint8_t Cpu::dec_r8() {
    bool half_carry = (registers_.*Reg & 0x0F) == 0x00;
    registers_.*Reg -= 1;

    registers_.set_flag(Registers::Flag::Z, registers_.*Reg == 0);
    registers_.set_flag(Registers::Flag::N, true);
    registers_.set_flag(Registers::Flag::H, half_carry);

    return 1;
}

uint8_t Cpu::dec_hl() {
    uint16_t address = registers_.get_hl();
    uint8_t val = mmu_.read(address);
    bool half_carry = (val & 0x0F) == 0x00;
    val -= 1;

    registers_.set_flag(Registers::Flag::Z, val == 0);
    registers_.set_flag(Registers::Flag::N, true);
    registers_.set_flag(Registers::Flag::H, half_carry);

    mmu_.write(address, val);

    return 3;
}

template<uint8_t Cpu::Registers::*Reg>
uint8_t Cpu::ld_r8_imm8() {
    uint8_t imm8 = mmu_.read(registers_.pc);
    registers_.pc++;
    registers_.*Reg = imm8;

    return 2;
}

uint8_t Cpu::ld_hl_imm8() {
    uint8_t imm8 = mmu_.read(registers_.pc);
    registers_.pc++;
    uint16_t address = registers_.get_hl();
    mmu_.write(address, imm8);

    return 3;
}

uint8_t Cpu::jr_imm8() {
    int8_t offset = static_cast<int8_t>(mmu_.read(registers_.pc));
    registers_.pc++;
    registers_.pc += offset;
    return 3;
}

template <Cpu::Registers::Flag Flag, bool ExpectedState>
uint8_t Cpu::jr_cond_imm8() {
    int8_t offset = static_cast<int8_t>(mmu_.read(registers_.pc));
    registers_.pc++;

    if (registers_.get_flag(Flag) == ExpectedState) {
        registers_.pc += offset;
        return 3;
    }

    return 2;
}

template <uint8_t Cpu::Registers::*Dst, uint8_t Cpu::Registers::*Src>
uint8_t Cpu::ld_r8_r8() {
    registers_.*Dst = registers_.*Src;
    return 1;
}

template<uint8_t Cpu::Registers::*Dst>
uint8_t Cpu::ld_r8_hl() {
    uint16_t address = registers_.get_hl();
    uint8_t val = mmu_.read(address);
    registers_.*Dst = val;
    return 2;
}

template<uint8_t Cpu::Registers::*Src>
uint8_t Cpu::ld_hl_r8() {
    uint16_t address = registers_.get_hl();
    mmu_.write(address, registers_.*Src);
    return 2;
}

template <uint8_t Cpu::Registers::*Reg>
uint8_t Cpu::add_a_r8() {
    uint8_t val = registers_.*Reg;
    uint8_t a = registers_.a;
    uint16_t res = static_cast<uint16_t>(a) + val;

    registers_.set_flag(Registers::Flag::Z, (res & 0x00FF) == 0);
    registers_.set_flag(Registers::Flag::N, false);
    registers_.set_flag(Registers::Flag::H, (((a & 0x0F) + (val & 0x0F)) & 0x10) != 0);
    registers_.set_flag(Registers::Flag::C, res > 0xFF);

    registers_.a = static_cast<uint8_t>(res);
    return 1;
}

uint8_t Cpu::add_a_hl() {
    uint8_t val = mmu_.read(registers_.get_hl());
    uint8_t a = registers_.a;
    uint16_t res = static_cast<uint16_t>(a) + val;

    registers_.set_flag(Registers::Flag::Z, (res & 0x00FF) == 0);
    registers_.set_flag(Registers::Flag::N, false);
    registers_.set_flag(Registers::Flag::H, (((a & 0x0F) + (val & 0x0F)) & 0x10) != 0);
    registers_.set_flag(Registers::Flag::C, res > 0xFF);

    registers_.a = static_cast<uint8_t>(res);
    return 2;
}

template <uint8_t Cpu::Registers::*Reg>
uint8_t Cpu::adc_a_r8() {
    uint8_t val = registers_.*Reg;
    uint8_t carry = registers_.get_flag(Registers::Flag::C) ? 1 : 0;
    uint8_t a = registers_.a;
    uint16_t res = static_cast<uint16_t>(a) + val + carry;

    registers_.set_flag(Registers::Flag::Z, (res & 0x00FF) == 0);
    registers_.set_flag(Registers::Flag::N, false);
    registers_.set_flag(Registers::Flag::H, (((a & 0x0F) + (val & 0x0F) + carry) & 0x10) != 0);
    registers_.set_flag(Registers::Flag::C, res > 0xFF);

    registers_.a = static_cast<uint8_t>(res);
    return 1;
}

uint8_t Cpu::adc_a_hl() {
    uint8_t val = mmu_.read(registers_.get_hl());
    uint8_t carry = registers_.get_flag(Registers::Flag::C) ? 1 : 0;
    uint8_t a = registers_.a;
    uint16_t res = static_cast<uint16_t>(a) + val + carry;

    registers_.set_flag(Registers::Flag::Z, (res & 0x00FF) == 0);
    registers_.set_flag(Registers::Flag::N, false);
    registers_.set_flag(Registers::Flag::H, (((a & 0x0F) + (val & 0x0F) + carry) & 0x10) != 0);
    registers_.set_flag(Registers::Flag::C, res > 0xFF);

    registers_.a = static_cast<uint8_t>(res);
    return 2;
}

template <uint8_t Cpu::Registers::*Reg>
uint8_t Cpu::sub_a_r8() {
    uint8_t val = registers_.*Reg;
    uint8_t a = registers_.a;
    uint16_t res = static_cast<uint16_t>(a) - val;

    registers_.set_flag(Registers::Flag::Z, (res & 0x00FF) == 0);
    registers_.set_flag(Registers::Flag::N, true);
    registers_.set_flag(Registers::Flag::H, (a & 0x0F) < (val & 0x0F));
    registers_.set_flag(Registers::Flag::C, a < val);

    registers_.a = static_cast<uint8_t>(res);
    return 1;
}

uint8_t Cpu::sub_a_hl() {
    uint8_t val = mmu_.read(registers_.get_hl());
    uint8_t a = registers_.a;
    uint16_t res = static_cast<uint16_t>(a) - val;

    registers_.set_flag(Registers::Flag::Z, (res & 0x00FF) == 0);
    registers_.set_flag(Registers::Flag::N, true);
    registers_.set_flag(Registers::Flag::H, (a & 0x0F) < (val & 0x0F));
    registers_.set_flag(Registers::Flag::C, a < val);

    registers_.a = static_cast<uint8_t>(res);
    return 2;
}

template <uint8_t Cpu::Registers::*Reg>
uint8_t Cpu::sbc_a_r8() {
    uint8_t val = registers_.*Reg;
    uint8_t carry = registers_.get_flag(Registers::Flag::C) ? 1 : 0;
    uint8_t a = registers_.a;
    uint16_t res = static_cast<uint16_t>(a) - val - carry;

    registers_.set_flag(Registers::Flag::Z, (res & 0x00FF) == 0);
    registers_.set_flag(Registers::Flag::N, true);
    registers_.set_flag(Registers::Flag::H, (a & 0x0F) < (val & 0x0F) + carry);
    registers_.set_flag(Registers::Flag::C, a < val + carry);

    registers_.a = static_cast<uint8_t>(res);
    return 1;
}

uint8_t Cpu::sbc_a_hl() {
    uint8_t val = mmu_.read(registers_.get_hl());
    uint8_t carry = registers_.get_flag(Registers::Flag::C) ? 1 : 0;
    uint8_t a = registers_.a;
    uint16_t res = static_cast<uint16_t>(a) - val - carry;

    registers_.set_flag(Registers::Flag::Z, (res & 0x00FF) == 0);
    registers_.set_flag(Registers::Flag::N, true);
    registers_.set_flag(Registers::Flag::H, (a & 0x0F) < (val & 0x0F) + carry);
    registers_.set_flag(Registers::Flag::C, a < val + carry);

    registers_.a = static_cast<uint8_t>(res);
    return 2;
}

template <uint8_t Cpu::Registers::*Reg>
uint8_t Cpu::and_a_r8() {
    uint8_t val = registers_.*Reg;
    registers_.a &= val;

    registers_.set_flag(Registers::Flag::Z, registers_.a == 0);
    registers_.set_flag(Registers::Flag::N, false);
    registers_.set_flag(Registers::Flag::H, true);
    registers_.set_flag(Registers::Flag::C, false);

    return 1;
}

uint8_t Cpu::and_a_hl() {
    uint8_t val = mmu_.read(registers_.get_hl());
    registers_.a &= val;

    registers_.set_flag(Registers::Flag::Z, registers_.a == 0);
    registers_.set_flag(Registers::Flag::N, false);
    registers_.set_flag(Registers::Flag::H, true);
    registers_.set_flag(Registers::Flag::C, false);

    return 2;
}

template <uint8_t Cpu::Registers::*Reg>
uint8_t Cpu::xor_a_r8() {
    uint8_t val = registers_.*Reg;
    registers_.a ^= val;

    registers_.set_flag(Registers::Flag::Z, registers_.a == 0);
    registers_.set_flag(Registers::Flag::N, false);
    registers_.set_flag(Registers::Flag::H, false);
    registers_.set_flag(Registers::Flag::C, false);

    return 1;
}

uint8_t Cpu::xor_a_hl() {
    uint8_t val = mmu_.read(registers_.get_hl());
    registers_.a ^= val;

    registers_.set_flag(Registers::Flag::Z, registers_.a == 0);
    registers_.set_flag(Registers::Flag::N, false);
    registers_.set_flag(Registers::Flag::H, false);
    registers_.set_flag(Registers::Flag::C, false);

    return 2;
}

template <uint8_t Cpu::Registers::*Reg>
uint8_t Cpu::or_a_r8() {
    uint8_t val = registers_.*Reg;
    registers_.a |= val;

    registers_.set_flag(Registers::Flag::Z, registers_.a == 0);
    registers_.set_flag(Registers::Flag::N, false);
    registers_.set_flag(Registers::Flag::H, false);
    registers_.set_flag(Registers::Flag::C, false);

    return 1;
}

uint8_t Cpu::or_a_hl() {
    uint8_t val = mmu_.read(registers_.get_hl());
    registers_.a |= val;

    registers_.set_flag(Registers::Flag::Z, registers_.a == 0);
    registers_.set_flag(Registers::Flag::N, false);
    registers_.set_flag(Registers::Flag::H, false);
    registers_.set_flag(Registers::Flag::C, false);

    return 2;
}

void Cpu::build_instruction_table() {
    instructions_.fill(&Cpu::unimplemented_instruction);

    /***********
     * Block 0 *
     ***********/

    instructions_[0x00] = &Cpu::nop;

    instructions_[0x01] = &Cpu::ld_r16_imm16<&Cpu::Registers::set_bc>;
    instructions_[0x11] = &Cpu::ld_r16_imm16<&Cpu::Registers::set_de>;
    instructions_[0x21] = &Cpu::ld_r16_imm16<&Cpu::Registers::set_hl>;
    instructions_[0x31] = &Cpu::ld_r16_imm16<&Cpu::Registers::set_sp>;

    instructions_[0x02] = &Cpu::ld_r16mem_a<&Cpu::Registers::get_bc>;
    instructions_[0x12] = &Cpu::ld_r16mem_a<&Cpu::Registers::get_de>;
    instructions_[0x22] = &Cpu::ld_hli_a;
    instructions_[0x32] = &Cpu::ld_hld_a;

    instructions_[0x0A] = &Cpu::ld_a_r16mem<&Cpu::Registers::get_bc>;
    instructions_[0x1A] = &Cpu::ld_a_r16mem<&Cpu::Registers::get_de>;
    instructions_[0x2A] = &Cpu::ld_a_hli;
    instructions_[0x3A] = &Cpu::ld_a_hld;

    instructions_[0x08] = &Cpu::ld_imm16_sp;

    instructions_[0x03] = &Cpu::inc_r16<&Cpu::Registers::get_bc, &Cpu::Registers::set_bc>;
    instructions_[0x13] = &Cpu::inc_r16<&Cpu::Registers::get_de, &Cpu::Registers::set_de>;
    instructions_[0x23] = &Cpu::inc_r16<&Cpu::Registers::get_hl, &Cpu::Registers::set_hl>;
    instructions_[0x33] = &Cpu::inc_r16<&Cpu::Registers::get_sp, &Cpu::Registers::set_sp>;

    instructions_[0x0B] = &Cpu::dec_r16<&Cpu::Registers::get_bc, &Cpu::Registers::set_bc>;
    instructions_[0x1B] = &Cpu::dec_r16<&Cpu::Registers::get_de, &Cpu::Registers::set_de>;
    instructions_[0x2B] = &Cpu::dec_r16<&Cpu::Registers::get_hl, &Cpu::Registers::set_hl>;
    instructions_[0x3B] = &Cpu::dec_r16<&Cpu::Registers::get_sp, &Cpu::Registers::set_sp>;

    instructions_[0x09] = &Cpu::add_hl_r16<&Cpu::Registers::get_bc>;
    instructions_[0x19] = &Cpu::add_hl_r16<&Cpu::Registers::get_de>;
    instructions_[0x29] = &Cpu::add_hl_r16<&Cpu::Registers::get_hl>;
    instructions_[0x39] = &Cpu::add_hl_r16<&Cpu::Registers::get_sp>;

    instructions_[0x04] = &Cpu::inc_r8<&Cpu::Registers::b>;
    instructions_[0x0C] = &Cpu::inc_r8<&Cpu::Registers::c>;
    instructions_[0x14] = &Cpu::inc_r8<&Cpu::Registers::d>;
    instructions_[0x1C] = &Cpu::inc_r8<&Cpu::Registers::e>;
    instructions_[0x24] = &Cpu::inc_r8<&Cpu::Registers::h>;
    instructions_[0x2C] = &Cpu::inc_r8<&Cpu::Registers::l>;
    instructions_[0x34] = &Cpu::inc_hl;
    instructions_[0x3C] = &Cpu::inc_r8<&Cpu::Registers::a>;

    instructions_[0x05] = &Cpu::dec_r8<&Cpu::Registers::b>;
    instructions_[0x0D] = &Cpu::dec_r8<&Cpu::Registers::c>;
    instructions_[0x15] = &Cpu::dec_r8<&Cpu::Registers::d>;
    instructions_[0x1D] = &Cpu::dec_r8<&Cpu::Registers::e>;
    instructions_[0x25] = &Cpu::dec_r8<&Cpu::Registers::h>;
    instructions_[0x2D] = &Cpu::dec_r8<&Cpu::Registers::l>;
    instructions_[0x35] = &Cpu::dec_hl;
    instructions_[0x3D] = &Cpu::dec_r8<&Cpu::Registers::a>;

    instructions_[0x06] = &Cpu::ld_r8_imm8<&Cpu::Registers::b>;
    instructions_[0x0E] = &Cpu::ld_r8_imm8<&Cpu::Registers::c>;
    instructions_[0x16] = &Cpu::ld_r8_imm8<&Cpu::Registers::d>;
    instructions_[0x1E] = &Cpu::ld_r8_imm8<&Cpu::Registers::e>;
    instructions_[0x26] = &Cpu::ld_r8_imm8<&Cpu::Registers::h>;
    instructions_[0x2E] = &Cpu::ld_r8_imm8<&Cpu::Registers::l>;
    instructions_[0x36] = &Cpu::ld_hl_imm8;
    instructions_[0x3E] = &Cpu::ld_r8_imm8<&Cpu::Registers::a>;

    instructions_[0x18] = &Cpu::jr_imm8;

    instructions_[0x20] = &Cpu::jr_cond_imm8<Cpu::Registers::Flag::Z, false>;
    instructions_[0x28] = &Cpu::jr_cond_imm8<Cpu::Registers::Flag::Z, true>;
    instructions_[0x30] = &Cpu::jr_cond_imm8<Cpu::Registers::Flag::C, false>;
    instructions_[0x38] = &Cpu::jr_cond_imm8<Cpu::Registers::Flag::C, true>;

    /***********
     * Block 0 *
     ***********/

    instructions_[0x40] = &Cpu::ld_r8_r8<&Cpu::Registers::b, &Cpu::Registers::b>;
    instructions_[0x41] = &Cpu::ld_r8_r8<&Cpu::Registers::b, &Cpu::Registers::c>;
    instructions_[0x42] = &Cpu::ld_r8_r8<&Cpu::Registers::b, &Cpu::Registers::d>;
    instructions_[0x43] = &Cpu::ld_r8_r8<&Cpu::Registers::b, &Cpu::Registers::e>;
    instructions_[0x44] = &Cpu::ld_r8_r8<&Cpu::Registers::b, &Cpu::Registers::h>;
    instructions_[0x45] = &Cpu::ld_r8_r8<&Cpu::Registers::b, &Cpu::Registers::l>;
    instructions_[0x46] = &Cpu::ld_r8_hl<&Cpu::Registers::b>;
    instructions_[0x47] = &Cpu::ld_r8_r8<&Cpu::Registers::b, &Cpu::Registers::a>;

    instructions_[0x48] = &Cpu::ld_r8_r8<&Cpu::Registers::c, &Cpu::Registers::b>;
    instructions_[0x49] = &Cpu::ld_r8_r8<&Cpu::Registers::c, &Cpu::Registers::c>;
    instructions_[0x4A] = &Cpu::ld_r8_r8<&Cpu::Registers::c, &Cpu::Registers::d>;
    instructions_[0x4B] = &Cpu::ld_r8_r8<&Cpu::Registers::c, &Cpu::Registers::e>;
    instructions_[0x4C] = &Cpu::ld_r8_r8<&Cpu::Registers::c, &Cpu::Registers::h>;
    instructions_[0x4D] = &Cpu::ld_r8_r8<&Cpu::Registers::c, &Cpu::Registers::l>;
    instructions_[0x4E] = &Cpu::ld_r8_hl<&Cpu::Registers::c>;
    instructions_[0x4F] = &Cpu::ld_r8_r8<&Cpu::Registers::c, &Cpu::Registers::a>;

    instructions_[0x50] = &Cpu::ld_r8_r8<&Cpu::Registers::d, &Cpu::Registers::b>;
    instructions_[0x51] = &Cpu::ld_r8_r8<&Cpu::Registers::d, &Cpu::Registers::c>;
    instructions_[0x52] = &Cpu::ld_r8_r8<&Cpu::Registers::d, &Cpu::Registers::d>;
    instructions_[0x53] = &Cpu::ld_r8_r8<&Cpu::Registers::d, &Cpu::Registers::e>;
    instructions_[0x54] = &Cpu::ld_r8_r8<&Cpu::Registers::d, &Cpu::Registers::h>;
    instructions_[0x55] = &Cpu::ld_r8_r8<&Cpu::Registers::d, &Cpu::Registers::l>;
    instructions_[0x56] = &Cpu::ld_r8_hl<&Cpu::Registers::d>;
    instructions_[0x57] = &Cpu::ld_r8_r8<&Cpu::Registers::d, &Cpu::Registers::a>;

    instructions_[0x58] = &Cpu::ld_r8_r8<&Cpu::Registers::e, &Cpu::Registers::b>;
    instructions_[0x59] = &Cpu::ld_r8_r8<&Cpu::Registers::e, &Cpu::Registers::c>;
    instructions_[0x5A] = &Cpu::ld_r8_r8<&Cpu::Registers::e, &Cpu::Registers::d>;
    instructions_[0x5B] = &Cpu::ld_r8_r8<&Cpu::Registers::e, &Cpu::Registers::e>;
    instructions_[0x5C] = &Cpu::ld_r8_r8<&Cpu::Registers::e, &Cpu::Registers::h>;
    instructions_[0x5D] = &Cpu::ld_r8_r8<&Cpu::Registers::e, &Cpu::Registers::l>;
    instructions_[0x5E] = &Cpu::ld_r8_hl<&Cpu::Registers::e>;
    instructions_[0x5F] = &Cpu::ld_r8_r8<&Cpu::Registers::e, &Cpu::Registers::a>;

    instructions_[0x60] = &Cpu::ld_r8_r8<&Cpu::Registers::h, &Cpu::Registers::b>;
    instructions_[0x61] = &Cpu::ld_r8_r8<&Cpu::Registers::h, &Cpu::Registers::c>;
    instructions_[0x62] = &Cpu::ld_r8_r8<&Cpu::Registers::h, &Cpu::Registers::d>;
    instructions_[0x63] = &Cpu::ld_r8_r8<&Cpu::Registers::h, &Cpu::Registers::e>;
    instructions_[0x64] = &Cpu::ld_r8_r8<&Cpu::Registers::h, &Cpu::Registers::h>;
    instructions_[0x65] = &Cpu::ld_r8_r8<&Cpu::Registers::h, &Cpu::Registers::l>;
    instructions_[0x66] = &Cpu::ld_r8_hl<&Cpu::Registers::h>;
    instructions_[0x67] = &Cpu::ld_r8_r8<&Cpu::Registers::h, &Cpu::Registers::a>;

    instructions_[0x68] = &Cpu::ld_r8_r8<&Cpu::Registers::l, &Cpu::Registers::b>;
    instructions_[0x69] = &Cpu::ld_r8_r8<&Cpu::Registers::l, &Cpu::Registers::c>;
    instructions_[0x6A] = &Cpu::ld_r8_r8<&Cpu::Registers::l, &Cpu::Registers::d>;
    instructions_[0x6B] = &Cpu::ld_r8_r8<&Cpu::Registers::l, &Cpu::Registers::e>;
    instructions_[0x6C] = &Cpu::ld_r8_r8<&Cpu::Registers::l, &Cpu::Registers::h>;
    instructions_[0x6D] = &Cpu::ld_r8_r8<&Cpu::Registers::l, &Cpu::Registers::l>;
    instructions_[0x6E] = &Cpu::ld_r8_hl<&Cpu::Registers::l>;
    instructions_[0x6F] = &Cpu::ld_r8_r8<&Cpu::Registers::l, &Cpu::Registers::a>;

    instructions_[0x70] = &Cpu::ld_hl_r8<&Cpu::Registers::b>;
    instructions_[0x71] = &Cpu::ld_hl_r8<&Cpu::Registers::c>;
    instructions_[0x72] = &Cpu::ld_hl_r8<&Cpu::Registers::d>;
    instructions_[0x73] = &Cpu::ld_hl_r8<&Cpu::Registers::e>;
    instructions_[0x74] = &Cpu::ld_hl_r8<&Cpu::Registers::h>;
    instructions_[0x75] = &Cpu::ld_hl_r8<&Cpu::Registers::l>;
    // TODO 0x76: HALT
    instructions_[0x77] = &Cpu::ld_hl_r8<&Cpu::Registers::a>;

    instructions_[0x78] = &Cpu::ld_r8_r8<&Cpu::Registers::a, &Cpu::Registers::b>;
    instructions_[0x79] = &Cpu::ld_r8_r8<&Cpu::Registers::a, &Cpu::Registers::c>;
    instructions_[0x7A] = &Cpu::ld_r8_r8<&Cpu::Registers::a, &Cpu::Registers::d>;
    instructions_[0x7B] = &Cpu::ld_r8_r8<&Cpu::Registers::a, &Cpu::Registers::e>;
    instructions_[0x7C] = &Cpu::ld_r8_r8<&Cpu::Registers::a, &Cpu::Registers::h>;
    instructions_[0x7D] = &Cpu::ld_r8_r8<&Cpu::Registers::a, &Cpu::Registers::l>;
    instructions_[0x7E] = &Cpu::ld_r8_hl<&Cpu::Registers::a>;
    instructions_[0x7F] = &Cpu::ld_r8_r8<&Cpu::Registers::a, &Cpu::Registers::a>;

    /***********
     * Block 0 *
     ***********/

    instructions_[0x80] = &Cpu::add_a_r8<&Cpu::Registers::b>;
    instructions_[0x81] = &Cpu::add_a_r8<&Cpu::Registers::c>;
    instructions_[0x82] = &Cpu::add_a_r8<&Cpu::Registers::d>;
    instructions_[0x83] = &Cpu::add_a_r8<&Cpu::Registers::e>;
    instructions_[0x84] = &Cpu::add_a_r8<&Cpu::Registers::h>;
    instructions_[0x85] = &Cpu::add_a_r8<&Cpu::Registers::l>;
    instructions_[0x86] = &Cpu::add_a_hl;
    instructions_[0x87] = &Cpu::add_a_r8<&Cpu::Registers::a>;

    instructions_[0x88] = &Cpu::adc_a_r8<&Cpu::Registers::b>;
    instructions_[0x89] = &Cpu::adc_a_r8<&Cpu::Registers::c>;
    instructions_[0x8A] = &Cpu::adc_a_r8<&Cpu::Registers::d>;
    instructions_[0x8B] = &Cpu::adc_a_r8<&Cpu::Registers::e>;
    instructions_[0x8C] = &Cpu::adc_a_r8<&Cpu::Registers::h>;
    instructions_[0x8D] = &Cpu::adc_a_r8<&Cpu::Registers::l>;
    instructions_[0x8E] = &Cpu::adc_a_hl;
    instructions_[0x8F] = &Cpu::adc_a_r8<&Cpu::Registers::a>;

    instructions_[0x90] = &Cpu::sub_a_r8<&Cpu::Registers::b>;
    instructions_[0x91] = &Cpu::sub_a_r8<&Cpu::Registers::c>;
    instructions_[0x92] = &Cpu::sub_a_r8<&Cpu::Registers::d>;
    instructions_[0x93] = &Cpu::sub_a_r8<&Cpu::Registers::e>;
    instructions_[0x94] = &Cpu::sub_a_r8<&Cpu::Registers::h>;
    instructions_[0x95] = &Cpu::sub_a_r8<&Cpu::Registers::l>;
    instructions_[0x96] = &Cpu::sub_a_hl;
    instructions_[0x97] = &Cpu::sub_a_r8<&Cpu::Registers::a>;

    instructions_[0x98] = &Cpu::sbc_a_r8<&Cpu::Registers::b>;
    instructions_[0x99] = &Cpu::sbc_a_r8<&Cpu::Registers::c>;
    instructions_[0x9A] = &Cpu::sbc_a_r8<&Cpu::Registers::d>;
    instructions_[0x9B] = &Cpu::sbc_a_r8<&Cpu::Registers::e>;
    instructions_[0x9C] = &Cpu::sbc_a_r8<&Cpu::Registers::h>;
    instructions_[0x9D] = &Cpu::sbc_a_r8<&Cpu::Registers::l>;
    instructions_[0x9E] = &Cpu::sbc_a_hl;
    instructions_[0x9F] = &Cpu::sbc_a_r8<&Cpu::Registers::a>;

    instructions_[0xA0] = &Cpu::and_a_r8<&Cpu::Registers::b>;
    instructions_[0xA1] = &Cpu::and_a_r8<&Cpu::Registers::c>;
    instructions_[0xA2] = &Cpu::and_a_r8<&Cpu::Registers::d>;
    instructions_[0xA3] = &Cpu::and_a_r8<&Cpu::Registers::e>;
    instructions_[0xA4] = &Cpu::and_a_r8<&Cpu::Registers::h>;
    instructions_[0xA5] = &Cpu::and_a_r8<&Cpu::Registers::l>;
    instructions_[0xA6] = &Cpu::and_a_hl;
    instructions_[0xA7] = &Cpu::and_a_r8<&Cpu::Registers::a>;

    instructions_[0xA8] = &Cpu::xor_a_r8<&Cpu::Registers::b>;
    instructions_[0xA9] = &Cpu::xor_a_r8<&Cpu::Registers::c>;
    instructions_[0xAA] = &Cpu::xor_a_r8<&Cpu::Registers::d>;
    instructions_[0xAB] = &Cpu::xor_a_r8<&Cpu::Registers::e>;
    instructions_[0xAC] = &Cpu::xor_a_r8<&Cpu::Registers::h>;
    instructions_[0xAD] = &Cpu::xor_a_r8<&Cpu::Registers::l>;
    instructions_[0xAE] = &Cpu::xor_a_hl;
    instructions_[0xAF] = &Cpu::xor_a_r8<&Cpu::Registers::a>;

    instructions_[0xB0] = &Cpu::or_a_r8<&Cpu::Registers::b>;
    instructions_[0xB1] = &Cpu::or_a_r8<&Cpu::Registers::c>;
    instructions_[0xB2] = &Cpu::or_a_r8<&Cpu::Registers::d>;
    instructions_[0xB3] = &Cpu::or_a_r8<&Cpu::Registers::e>;
    instructions_[0xB4] = &Cpu::or_a_r8<&Cpu::Registers::h>;
    instructions_[0xB5] = &Cpu::or_a_r8<&Cpu::Registers::l>;
    instructions_[0xB6] = &Cpu::or_a_hl;
    instructions_[0xB7] = &Cpu::or_a_r8<&Cpu::Registers::a>;
}
