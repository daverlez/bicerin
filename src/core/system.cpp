#include "core/system.h"

System::System(const std::string& rom_path)
    : cartridge(Cartridge::load(rom_path)),
      timer(),
      bus(timer, cartridge.get()),
      cpu()
{
    if (!cartridge) {
        throw std::runtime_error("Cannot load rom.");
    }
}

void System::run() {
    while (true) {
        uint8_t cycles = cpu.step(bus);
        timer.tick(cycles);

        if (timer.is_interrupt_requested()) {
            bus.request_interrupt(2);
            timer.clear_interrupt_request();
        }
    }
}