#pragma once
#include <cstdint>

class Joypad {
public:
    // Enum pulita da esporre al frontend
    enum class Button {
        Right, Left, Up, Down,
        A, B, Select, Start
    };

    Joypad() = default;
    ~Joypad() = default;

    /// To be called from the frontend when a button is pressed.
    void press(Button button);
    /// To be called from the frontend when a button is released.
    void release(Button button);

    /// Handles reads from Bus (0xFF00).
    uint8_t read() const;
    /// Handles writes from Bus (0xFF00).
    void write(uint8_t value);

    bool is_interrupt_requested() const { return interrupt_requested; }
    void clear_interrupt_request() { interrupt_requested = false; }

private:
    uint8_t action_state{0x0F};    // Start, Select, B, A
    uint8_t direction_state{0x0F}; // Down, Up, Left, Right

    uint8_t selection{0x30}; 

    bool interrupt_requested{false};

    uint8_t get_current_state() const;
};