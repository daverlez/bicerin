#include "core/ppu.h"

Ppu::Ppu() {
    vram.fill(0x00);
    oam.fill(0x00);
}

uint8_t Ppu::read(uint16_t address) const {
    if (address >= 0x8000 && address <= 0x9FFF)
        return vram[address - 0x8000];

    if (address >= 0xFE00 && address <= 0xFE9F)
        return oam[address - 0xFE00];
    
    switch (address) {
        case 0xFF40: return lcdc;
        case 0xFF41: return stat;
        case 0xFF42: return scy;
        case 0xFF43: return scx;
        case 0xFF44: return ly;
        case 0xFF45: return lyc;
        case 0xFF47: return bgp;
        case 0xFF48: return obp0;
        case 0xFF49: return obp1;
        case 0xFF4A: return wy;
        case 0xFF4B: return wx;
        default: return 0xFF;
    }
}

void Ppu::write(uint16_t address, uint8_t value) {
    if (address >= 0x8000 && address <= 0x9FFF) {
        vram[address - 0x8000] = value;
        return;
    }
    if (address >= 0xFE00 && address <= 0xFE9F) {
        oam[address - 0xFE00] = value;
        return;
    }

    switch (address) {
        case 0xFF40: lcdc = value; break;
        case 0xFF41:
            stat = (value & 0xF8) | (stat & 0x07); 
            break;
        case 0xFF42: scy = value; break;
        case 0xFF43: scx = value; break;
        case 0xFF44:
            // TODO: on real hw resets counter
            break; 
        case 0xFF45: lyc = value; break;
        case 0xFF47: bgp = value; break;
        case 0xFF48: obp0 = value; break;
        case 0xFF49: obp1 = value; break;
        case 0xFF4A: wy = value; break;
        case 0xFF4B: wx = value; break;
    }
}

void Ppu::tick([[maybe_unused]]  uint8_t cycles) {
    // TODO
}