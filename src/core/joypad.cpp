#include "core/joypad.h"

void Joypad::press(Button button) {
    uint8_t old_state = get_current_state();

    switch (button) {
        case Button::Right:  direction_state &= ~(1 << 0); break;
        case Button::Left:   direction_state &= ~(1 << 1); break;
        case Button::Up:     direction_state &= ~(1 << 2); break;
        case Button::Down:   direction_state &= ~(1 << 3); break;
        
        case Button::A:      action_state &= ~(1 << 0); break;
        case Button::B:      action_state &= ~(1 << 1); break;
        case Button::Select: action_state &= ~(1 << 2); break;
        case Button::Start:  action_state &= ~(1 << 3); break;
    }

    uint8_t new_state = get_current_state();

    if ((old_state & ~new_state) & 0x0F)
        interrupt_requested = true;
}

void Joypad::release(Button button) {
    switch (button) {
        case Button::Right:  direction_state |= (1 << 0); break;
        case Button::Left:   direction_state |= (1 << 1); break;
        case Button::Up:     direction_state |= (1 << 2); break;
        case Button::Down:   direction_state |= (1 << 3); break;
        
        case Button::A:      action_state |= (1 << 0); break;
        case Button::B:      action_state |= (1 << 1); break;
        case Button::Select: action_state |= (1 << 2); break;
        case Button::Start:  action_state |= (1 << 3); break;
    }
}

uint8_t Joypad::get_current_state() const {
    uint8_t output = 0x0F;

    // Bit 5 (0x20): action
    if ((selection & 0x20) == 0)
        output &= action_state;
    
    // Bit 4 (0x10): direction
    if ((selection & 0x10) == 0) {
        output &= direction_state;
    }

    return output;
}

uint8_t Joypad::read() const {
    // I bit 6 e 7 restituiscono quasi sempre 1 sull'hardware originale.
    // Assembliamo l'output con la selection corrente.
    return 0xC0 | selection | get_current_state();
}

void Joypad::write(uint8_t value) {
    uint8_t old_state = get_current_state();

    selection = value & 0x30;
    uint8_t new_state = get_current_state();

    if ((old_state & ~new_state) & 0x0F)
        interrupt_requested = true;
}