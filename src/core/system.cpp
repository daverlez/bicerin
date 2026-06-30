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
}

void System::run() {
    while (true) {
        uint8_t cycles = cpu.step(bus);
        timer.tick(cycles);

        if (timer.is_interrupt_requested()) {
            bus.request_interrupt(2);
            timer.clear_interrupt_request();
        }

        if (joypad.is_interrupt_requested()) {
            bus.request_interrupt(4);
            joypad.clear_interrupt_request();
        }
    }
}