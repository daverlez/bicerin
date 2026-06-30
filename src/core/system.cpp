#include "core/system.h"

System::System() : timer(), bus(timer), cpu() {}

bool System::load_rom(const std::string& filepath) {
    return bus.load_rom(filepath);
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