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
        case 0xFF40: {
            bool was_on = (lcdc & 0x80) != 0;
            bool is_on = (value & 0x80) != 0;
            lcdc = value;

            if (was_on && !is_on) {
                ly = 0;
                cycles_accumulator = 0;
                stat = (stat & 0xFC) | static_cast<uint8_t>(Mode::HBlank);
            }
            break;
        }
        case 0xFF41:
            stat = (value & 0xF8) | (stat & 0x07); 
            break;
        case 0xFF42: scy = value; break;
        case 0xFF43: scx = value; break;
        case 0xFF44:
            // Read only
            break; 
        case 0xFF45: lyc = value; break;
        case 0xFF47: bgp = value; break;
        case 0xFF48: obp0 = value; break;
        case 0xFF49: obp1 = value; break;
        case 0xFF4A: wy = value; break;
        case 0xFF4B: wx = value; break;
    }
}

void Ppu::tick(uint8_t m_cycles) {
    if ((lcdc & 0x80) == 0)
        return;

    cycles_accumulator += m_cycles;
    Mode current_mode = static_cast<Mode>(stat & 0x03);

    switch (current_mode) {
        case Mode::OamSearch:
            if (cycles_accumulator >= 20) {
                cycles_accumulator -= 20;
                change_mode(Mode::PixelTransfer);
            }
            break;

        case Mode::PixelTransfer:
            if (cycles_accumulator >= 43) {
                cycles_accumulator -= 43;

                // TODO: Scanline rendering

                change_mode(Mode::HBlank);
            }
            break;

        case Mode::HBlank:
            if (cycles_accumulator >= 51) {
                cycles_accumulator -= 51;
                ly++;
                check_lyc();

                if (ly == 144) {
                    change_mode(Mode::VBlank);
                    vblank_interrupt_requested = true;
                } else {
                    change_mode(Mode::OamSearch);
                }
            }
            break;

        case Mode::VBlank:
            if (cycles_accumulator >= 114) {
                cycles_accumulator -= 114;
                ly++;
                check_lyc();

                if (ly > 153) {
                    ly = 0;
                    check_lyc();
                    change_mode(Mode::OamSearch);
                }
            }
            break;
    }
}

void Ppu::change_mode(Mode new_mode) {
    stat = (stat & 0xFC) | static_cast<uint8_t>(new_mode);

    bool request_interrupt = false;
    if (new_mode == Mode::HBlank && (stat & (1 << 3))) request_interrupt = true;
    if (new_mode == Mode::VBlank && (stat & (1 << 4))) request_interrupt = true;
    if (new_mode == Mode::OamSearch && (stat & (1 << 5))) request_interrupt = true;

    if (request_interrupt)
        stat_interrupt_requested = true;
}

void Ppu::check_lyc() {
    if (ly == lyc) {
        stat |= (1 << 2);
        if (stat & (1 << 6)) {
            stat_interrupt_requested = true;
        }
    } else {
        stat &= ~(1 << 2);
    }
}