#pragma once
#include <cstdint>

#include "core/bus.h"

class Cpu {
public:
    Cpu();
    ~Cpu() = default;

    /// Resets the CPU to the initial state.
    void reset();

    /// Performs a fetch-decode-execute step.
    void step(Bus& bus);

    uint8_t a{0};
    uint8_t f{0};

    uint8_t b{0};
    uint8_t c{0};

    uint8_t d{0};
    uint8_t e{0};

    uint8_t h{0};
    uint8_t l{0};

    uint16_t sp{0};
    uint16_t pc{0};

    uint16_t get_af() const { return (static_cast<uint16_t>(a) << 8) | f; }
    uint16_t get_bc() const { return (static_cast<uint16_t>(b) << 8) | c; }
    uint16_t get_de() const { return (static_cast<uint16_t>(d) << 8) | e; }
    uint16_t get_hl() const { return (static_cast<uint16_t>(h) << 8) | l; }

    void set_af(uint16_t val) { a = static_cast<uint8_t>(val >> 8); f = static_cast<uint8_t>(val & 0xF0); }
    void set_bc(uint16_t val) { b = static_cast<uint8_t>(val >> 8); c = static_cast<uint8_t>(val & 0xFF); }
    void set_de(uint16_t val) { d = static_cast<uint8_t>(val >> 8); e = static_cast<uint8_t>(val & 0xFF); }
    void set_hl(uint16_t val) { h = static_cast<uint8_t>(val >> 8); l = static_cast<uint8_t>(val & 0xFF); }

private:
    uint8_t fetch(Bus& bus);
    void execute(uint8_t opcode, Bus& bus);

    /// Maps an index (0-8) to its register to write a value.
    void set_reg8(uint8_t index, uint8_t value, Bus& bus);
    /// Maps an index (0-8) to its register to read a value.
    uint8_t get_reg8(uint8_t index, Bus& bus) const;
};