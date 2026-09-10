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

void Cpu::build_instruction_table() {
    instructions_.fill(&Cpu::unimplemented_instruction);

    instructions_[0x00] = &Cpu::nop;

    instructions_[0x04] = &Cpu::inc_r8<&Cpu::Registers::b>;
    instructions_[0x0C] = &Cpu::inc_r8<&Cpu::Registers::c>;
    instructions_[0x14] = &Cpu::inc_r8<&Cpu::Registers::d>;
    instructions_[0x1C] = &Cpu::inc_r8<&Cpu::Registers::e>;
    instructions_[0x24] = &Cpu::inc_r8<&Cpu::Registers::h>;
    instructions_[0x2C] = &Cpu::inc_r8<&Cpu::Registers::l>;
    instructions_[0x34] = &Cpu::inc_hl;
    instructions_[0x3C] = &Cpu::inc_r8<&Cpu::Registers::a>;
}
