#include "core/cpu.h"

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