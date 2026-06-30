#include <gtest/gtest.h>
#include "core/joypad.h"

class JoypadTest : public ::testing::Test {
protected:
    Joypad joypad;

    void SetUp() override {
        joypad.write(0x30);
        joypad.clear_interrupt_request();
    }
};

TEST_F(JoypadTest, InitialStateReturnsAllReleased) {
    joypad.write(0x20);
    EXPECT_EQ(joypad.read(), 0xEF);

    joypad.write(0x10);
    EXPECT_EQ(joypad.read(), 0xDF);
}

TEST_F(JoypadTest, PressActionButtons) {
    joypad.write(0x10);

    joypad.press(Joypad::Button::A);
    EXPECT_EQ(joypad.read() & 0x0F, 0x0E);

    joypad.press(Joypad::Button::Start);
    EXPECT_EQ(joypad.read() & 0x0F, 0x06);

    joypad.release(Joypad::Button::A);
    EXPECT_EQ(joypad.read() & 0x0F, 0x07);
}

TEST_F(JoypadTest, MatrixIsolation) {
    joypad.press(Joypad::Button::Down);

    joypad.write(0x10);
    EXPECT_EQ(joypad.read() & 0x0F, 0x0F);

    joypad.write(0x20);
    EXPECT_EQ(joypad.read() & 0x0F, 0x07);
}

TEST_F(JoypadTest, InterruptOnButtonPress) {
    joypad.write(0x10);
    EXPECT_FALSE(joypad.is_interrupt_requested());

    joypad.press(Joypad::Button::B);
    EXPECT_TRUE(joypad.is_interrupt_requested());

    joypad.clear_interrupt_request();

    joypad.release(Joypad::Button::B);
    EXPECT_FALSE(joypad.is_interrupt_requested());
}

TEST_F(JoypadTest, InterruptOnSelectionChange) {
    joypad.write(0x30);

    joypad.press(Joypad::Button::A);
    EXPECT_FALSE(joypad.is_interrupt_requested());

    joypad.write(0x10);
    EXPECT_TRUE(joypad.is_interrupt_requested());
}