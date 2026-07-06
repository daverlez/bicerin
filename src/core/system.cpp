#include "core/system.h"

System::System(const std::string& rom_path)
    : cartridge(Cartridge::load(rom_path)),
      timer(),
      bus(),
      cpu(),
      joypad()
{
    if (!cartridge)
        throw std::runtime_error("Cannot load rom.");

    bus.connect_timer(&timer);
    bus.connect_cartridge(cartridge.get());
    bus.connect_joypad(&joypad);
    bus.connect_ppu(&ppu);
}

void System::run_frame() {
    uint32_t cycles_this_frame = 0;
    const uint32_t MAX_CYCLES = 17556;

    while (cycles_this_frame < MAX_CYCLES) {
        uint8_t cycles = cpu.step(bus);

        timer.tick(cycles);
        ppu.tick(cycles);

        if (timer.is_interrupt_requested()) {
            bus.request_interrupt(2);
            timer.clear_interrupt_request();
        }

        if (joypad.is_interrupt_requested()) {
            bus.request_interrupt(4);
            joypad.clear_interrupt_request();
        }

        if (ppu.is_vblank_interrupt_requested()) {
            bus.request_interrupt(0);
            ppu.clear_vblank_interrupt();
        }

        if (ppu.is_stat_interrupt_requested()) {
            bus.request_interrupt(1);
            ppu.clear_stat_interrupt();
        }

        cycles_this_frame += cycles;
    }
}