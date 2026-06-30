#include <gtest/gtest.h>
#include "core/timer.h"

class TimerTest : public ::testing::Test {
protected:
    Timer timer;

    void SetUp() override {
        timer.reset();
    }
};

TEST_F(TimerTest, InitialState) {
    EXPECT_EQ(timer.read_reg(0xFF04), 0x00);
    EXPECT_EQ(timer.read_reg(0xFF05), 0x00);
    EXPECT_EQ(timer.read_reg(0xFF06), 0x00);
    EXPECT_EQ(timer.read_reg(0xFF07), 0xF8);
    EXPECT_FALSE(timer.is_interrupt_requested());
}

TEST_F(TimerTest, DivIncrementsCorrectly) {
    timer.tick(63);
    EXPECT_EQ(timer.read_reg(0xFF04), 0x00);

    timer.tick(1);
    EXPECT_EQ(timer.read_reg(0xFF04), 0x01);
}

TEST_F(TimerTest, DivResetOnWrite) {
    timer.tick(64);
    EXPECT_EQ(timer.read_reg(0xFF04), 0x01);

    timer.write_reg(0xFF04, 0x42);
    EXPECT_EQ(timer.read_reg(0xFF04), 0x00);
}

TEST_F(TimerTest, TimaIncrementsAt4096Hz) {
    timer.write_reg(0xFF07, 0x04);

    timer.tick(255);
    EXPECT_EQ(timer.read_reg(0xFF05), 0x00);

    timer.tick(1);
    EXPECT_EQ(timer.read_reg(0xFF05), 0x01);
}

TEST_F(TimerTest, TimaDoesNotIncrementWhenDisabled) {
    timer.write_reg(0xFF07, 0x00);

    timer.tick(255);
    timer.tick(1);
    
    EXPECT_EQ(timer.read_reg(0xFF05), 0x00);
    EXPECT_EQ(timer.read_reg(0xFF04), 0x04);
}

TEST_F(TimerTest, TimaOverflowTriggersInterruptAndReloadsTma) {
    timer.write_reg(0xFF06, 0x42);
    timer.write_reg(0xFF05, 0xFF);
    timer.write_reg(0xFF07, 0x05);

    EXPECT_FALSE(timer.is_interrupt_requested());

    timer.tick(4);
    EXPECT_EQ(timer.read_reg(0xFF05), 0x42);
    EXPECT_TRUE(timer.is_interrupt_requested());
}

TEST_F(TimerTest, ClearInterruptRequest) {
    timer.write_reg(0xFF05, 0xFF); 
    timer.write_reg(0xFF07, 0x05); 
    timer.tick(4);

    EXPECT_TRUE(timer.is_interrupt_requested());
    
    timer.clear_interrupt_request();
    EXPECT_FALSE(timer.is_interrupt_requested());
}