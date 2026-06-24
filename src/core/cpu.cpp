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