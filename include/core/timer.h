#pragma once
#include <cstdint>

class Timer {
public:
    Timer();
    ~Timer() = default;

    /// Resets the timer to the initial state of the system.
    void reset();

    /// Advances the timer basing on the M-cycles spent on the CPU.
    void tick(uint8_t m_cycles);

    /// Handles reads from bus in the range 0xFF04-0xFF07.
    uint8_t read_reg(uint16_t address) const;
    /// Handles writes from bus in the range 0xFF04-0xFF07.
    void write_reg(uint16_t address, uint8_t value);

    /// Checks if the timer requested an interrupt.
    bool is_interrupt_requested() const { return interrupt_requested; }
    /// Resets the interrupt request flag after serving.
    void clear_interrupt_request() { interrupt_requested = false; }

private:
    uint16_t internal_counter{0};   // 0xFF04 (8 most significant bits are DIVA)
    uint8_t tima{0};                // 0xFF05 (TIMA)
    uint8_t tma{0};                 // 0xFF06 (TMA)
    uint8_t tac{0};                 // 0xFF07 (TAC)

    bool interrupt_requested{false};

    /// Returns true if the timer is enabled in the TAC register.
    bool is_enabled() const { return (tac & 0x04) != 0; }

    /// Gets the current clock divider basing on the 0-1 bits in TAC.
    uint16_t get_clock_divider() const;
};