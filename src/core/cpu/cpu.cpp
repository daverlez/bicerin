#include "cpu.h"

Cpu::Cpu() {
    reset();
}

void Cpu::reset() {
    registers_.a = 0x00;
    registers_.b = 0x00;
    registers_.c = 0x00;
    registers_.d = 0x00;
    registers_.e = 0x00;
    registers_.h = 0x00;
    registers_.l = 0x00;
    registers_.sp = 0x0000;
    registers_.pc = 0x0100;
}

auto Cpu::tick() -> uint8_t {
    // TODO
    return 1;
}
