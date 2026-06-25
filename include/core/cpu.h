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

    bool ime{false};

    uint16_t get_af() const { return (static_cast<uint16_t>(a) << 8) | f; }
    uint16_t get_bc() const { return (static_cast<uint16_t>(b) << 8) | c; }
    uint16_t get_de() const { return (static_cast<uint16_t>(d) << 8) | e; }
    uint16_t get_hl() const { return (static_cast<uint16_t>(h) << 8) | l; }

    void set_af(uint16_t val) { a = static_cast<uint8_t>(val >> 8); f = static_cast<uint8_t>(val & 0xF0); }
    void set_bc(uint16_t val) { b = static_cast<uint8_t>(val >> 8); c = static_cast<uint8_t>(val & 0xFF); }
    void set_de(uint16_t val) { d = static_cast<uint8_t>(val >> 8); e = static_cast<uint8_t>(val & 0xFF); }
    void set_hl(uint16_t val) { h = static_cast<uint8_t>(val >> 8); l = static_cast<uint8_t>(val & 0xFF); }

private:
    /// Reads a byte from memory and advances the PC.
    uint8_t fetch8(Bus& bus);
    /// Reads a byte from memory (little-endian) and advances the PC by two.
    uint16_t fetch16(Bus& bus);

    /// Decodes the opcode and performs the related execution.
    void execute(uint8_t opcode, Bus& bus);
    /// Decodes the opcode with prefix and performs the related execution.
    void execute_cb(uint8_t cb_opcode, Bus& bus);

    /// Maps an index (0-8) to its register to write a value.
    void set_reg8(uint8_t index, uint8_t value, Bus& bus);
    /// Maps an index (0-8) to its register to read a value.
    uint8_t get_reg8(uint8_t index, Bus& bus) const;

    /// Maps an index (0-3) to the corresponding reg16stk (see pandocs) to write a value.
    void set_r16stk(uint8_t index, uint16_t value);
    /// Maps an index (0-3) to the corresponding reg16stk (see pandocs) to read a value.
    uint16_t get_r16stk(uint8_t index) const;

    /// Stack operation: pushes two bytes on the stack.
    void push(uint16_t value, Bus& bus);
    /// Stack operation: pops two bytes from the stack.
    uint16_t pop(Bus& bus);

    /// Checks a condition (0-3) basing on flags Z and C.
    bool check_cond(uint8_t cond) const;

    bool get_flag_z() const { return (f & 0x80) != 0; }
    bool get_flag_n() const { return (f & 0x40) != 0; }
    bool get_flag_h() const { return (f & 0x20) != 0; }
    bool get_flag_c() const { return (f & 0x10) != 0; }

    void set_flag_z(bool value) { if (value) f |= 0x80; else f &= ~0x80; }
    void set_flag_n(bool value) { if (value) f |= 0x40; else f &= ~0x40; }
    void set_flag_h(bool value) { if (value) f |= 0x20; else f &= ~0x20; }
    void set_flag_c(bool value) { if (value) f |= 0x10; else f &= ~0x10; }

    void add_a(uint8_t value);
    void adc_a(uint8_t value);
    void sub_a(uint8_t value);
    void sbc_a(uint8_t value);
    void and_a(uint8_t value);
    void xor_a(uint8_t value);
    void or_a(uint8_t value);
    void cp_a(uint8_t value);
};