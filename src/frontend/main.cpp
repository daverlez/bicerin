#include <iostream>
#include "core/system.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom_path.gb>\n";
        return -1;
    }

    System system;

    // Carichiamo la ROM
    if (!system.load_rom(argv[1])) {
        return -1;
    }

    std::cout << "Starting emulation...\n";
    std::cout << "---------------------\n";

    system.run();
}