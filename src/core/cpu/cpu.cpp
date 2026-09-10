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

void Cpu::build_instruction_table() {
    instructions_.fill(&Cpu::unimplemented_instruction);

    /***********
     * Block 0 *
     ***********/

    instructions_[0x00] = &Cpu::nop;

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

    /***********
     * Block 0 *
     ***********/

    // LD B, r
    instructions_[0x40] = &Cpu::ld_r8_r8<&Cpu::Registers::b, &Cpu::Registers::b>;
    instructions_[0x41] = &Cpu::ld_r8_r8<&Cpu::Registers::b, &Cpu::Registers::c>;
    instructions_[0x42] = &Cpu::ld_r8_r8<&Cpu::Registers::b, &Cpu::Registers::d>;
    instructions_[0x43] = &Cpu::ld_r8_r8<&Cpu::Registers::b, &Cpu::Registers::e>;
    instructions_[0x44] = &Cpu::ld_r8_r8<&Cpu::Registers::b, &Cpu::Registers::h>;
    instructions_[0x45] = &Cpu::ld_r8_r8<&Cpu::Registers::b, &Cpu::Registers::l>;
    instructions_[0x46] = &Cpu::ld_r8_hl<&Cpu::Registers::b>;
    instructions_[0x47] = &Cpu::ld_r8_r8<&Cpu::Registers::b, &Cpu::Registers::a>;

    // LD C, r
    instructions_[0x48] = &Cpu::ld_r8_r8<&Cpu::Registers::c, &Cpu::Registers::b>;
    instructions_[0x49] = &Cpu::ld_r8_r8<&Cpu::Registers::c, &Cpu::Registers::c>;
    instructions_[0x4A] = &Cpu::ld_r8_r8<&Cpu::Registers::c, &Cpu::Registers::d>;
    instructions_[0x4B] = &Cpu::ld_r8_r8<&Cpu::Registers::c, &Cpu::Registers::e>;
    instructions_[0x4C] = &Cpu::ld_r8_r8<&Cpu::Registers::c, &Cpu::Registers::h>;
    instructions_[0x4D] = &Cpu::ld_r8_r8<&Cpu::Registers::c, &Cpu::Registers::l>;
    instructions_[0x4E] = &Cpu::ld_r8_hl<&Cpu::Registers::c>;
    instructions_[0x4F] = &Cpu::ld_r8_r8<&Cpu::Registers::c, &Cpu::Registers::a>;

    // LD D, r
    instructions_[0x50] = &Cpu::ld_r8_r8<&Cpu::Registers::d, &Cpu::Registers::b>;
    instructions_[0x51] = &Cpu::ld_r8_r8<&Cpu::Registers::d, &Cpu::Registers::c>;
    instructions_[0x52] = &Cpu::ld_r8_r8<&Cpu::Registers::d, &Cpu::Registers::d>;
    instructions_[0x53] = &Cpu::ld_r8_r8<&Cpu::Registers::d, &Cpu::Registers::e>;
    instructions_[0x54] = &Cpu::ld_r8_r8<&Cpu::Registers::d, &Cpu::Registers::h>;
    instructions_[0x55] = &Cpu::ld_r8_r8<&Cpu::Registers::d, &Cpu::Registers::l>;
    instructions_[0x56] = &Cpu::ld_r8_hl<&Cpu::Registers::d>;
    instructions_[0x57] = &Cpu::ld_r8_r8<&Cpu::Registers::d, &Cpu::Registers::a>;

    // LD E, r
    instructions_[0x58] = &Cpu::ld_r8_r8<&Cpu::Registers::e, &Cpu::Registers::b>;
    instructions_[0x59] = &Cpu::ld_r8_r8<&Cpu::Registers::e, &Cpu::Registers::c>;
    instructions_[0x5A] = &Cpu::ld_r8_r8<&Cpu::Registers::e, &Cpu::Registers::d>;
    instructions_[0x5B] = &Cpu::ld_r8_r8<&Cpu::Registers::e, &Cpu::Registers::e>;
    instructions_[0x5C] = &Cpu::ld_r8_r8<&Cpu::Registers::e, &Cpu::Registers::h>;
    instructions_[0x5D] = &Cpu::ld_r8_r8<&Cpu::Registers::e, &Cpu::Registers::l>;
    instructions_[0x5E] = &Cpu::ld_r8_hl<&Cpu::Registers::e>;
    instructions_[0x5F] = &Cpu::ld_r8_r8<&Cpu::Registers::e, &Cpu::Registers::a>;

    // LD H, r
    instructions_[0x60] = &Cpu::ld_r8_r8<&Cpu::Registers::h, &Cpu::Registers::b>;
    instructions_[0x61] = &Cpu::ld_r8_r8<&Cpu::Registers::h, &Cpu::Registers::c>;
    instructions_[0x62] = &Cpu::ld_r8_r8<&Cpu::Registers::h, &Cpu::Registers::d>;
    instructions_[0x63] = &Cpu::ld_r8_r8<&Cpu::Registers::h, &Cpu::Registers::e>;
    instructions_[0x64] = &Cpu::ld_r8_r8<&Cpu::Registers::h, &Cpu::Registers::h>;
    instructions_[0x65] = &Cpu::ld_r8_r8<&Cpu::Registers::h, &Cpu::Registers::l>;
    instructions_[0x66] = &Cpu::ld_r8_hl<&Cpu::Registers::h>;
    instructions_[0x67] = &Cpu::ld_r8_r8<&Cpu::Registers::h, &Cpu::Registers::a>;

    // LD L, r
    instructions_[0x68] = &Cpu::ld_r8_r8<&Cpu::Registers::l, &Cpu::Registers::b>;
    instructions_[0x69] = &Cpu::ld_r8_r8<&Cpu::Registers::l, &Cpu::Registers::c>;
    instructions_[0x6A] = &Cpu::ld_r8_r8<&Cpu::Registers::l, &Cpu::Registers::d>;
    instructions_[0x6B] = &Cpu::ld_r8_r8<&Cpu::Registers::l, &Cpu::Registers::e>;
    instructions_[0x6C] = &Cpu::ld_r8_r8<&Cpu::Registers::l, &Cpu::Registers::h>;
    instructions_[0x6D] = &Cpu::ld_r8_r8<&Cpu::Registers::l, &Cpu::Registers::l>;
    instructions_[0x6E] = &Cpu::ld_r8_hl<&Cpu::Registers::l>;
    instructions_[0x6F] = &Cpu::ld_r8_r8<&Cpu::Registers::l, &Cpu::Registers::a>;

    // LD [HL], r and HALT (0x76).
    instructions_[0x70] = &Cpu::ld_hl_r8<&Cpu::Registers::b>;
    instructions_[0x71] = &Cpu::ld_hl_r8<&Cpu::Registers::c>;
    instructions_[0x72] = &Cpu::ld_hl_r8<&Cpu::Registers::d>;
    instructions_[0x73] = &Cpu::ld_hl_r8<&Cpu::Registers::e>;
    instructions_[0x74] = &Cpu::ld_hl_r8<&Cpu::Registers::h>;
    instructions_[0x75] = &Cpu::ld_hl_r8<&Cpu::Registers::l>;
    // TODO 0x76: HALT
    instructions_[0x77] = &Cpu::ld_hl_r8<&Cpu::Registers::a>;

    // LD A, r
    instructions_[0x78] = &Cpu::ld_r8_r8<&Cpu::Registers::a, &Cpu::Registers::b>;
    instructions_[0x79] = &Cpu::ld_r8_r8<&Cpu::Registers::a, &Cpu::Registers::c>;
    instructions_[0x7A] = &Cpu::ld_r8_r8<&Cpu::Registers::a, &Cpu::Registers::d>;
    instructions_[0x7B] = &Cpu::ld_r8_r8<&Cpu::Registers::a, &Cpu::Registers::e>;
    instructions_[0x7C] = &Cpu::ld_r8_r8<&Cpu::Registers::a, &Cpu::Registers::h>;
    instructions_[0x7D] = &Cpu::ld_r8_r8<&Cpu::Registers::a, &Cpu::Registers::l>;
    instructions_[0x7E] = &Cpu::ld_r8_hl<&Cpu::Registers::a>;
    instructions_[0x7F] = &Cpu::ld_r8_r8<&Cpu::Registers::a, &Cpu::Registers::a>;
}
