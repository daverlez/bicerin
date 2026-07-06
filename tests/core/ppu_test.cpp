#include <gtest/gtest.h>
#include "core/ppu.h"

class PpuTest : public ::testing::Test {
protected:
    Ppu ppu;

    void SetUp() override {
        ppu.write(0xFF40, 0x91); 
        ppu.write(0xFF41, 0x00);
        ppu.write(0xFF45, 0x00);

        ppu.write(0xFF40, 0x00);
        ppu.write(0xFF40, 0x91);
    }
};

TEST_F(PpuTest, LcdOffDoesNotTick) {
    ppu.write(0xFF40, 0x00);
    ppu.tick(100);
    EXPECT_EQ(ppu.read(0xFF44), 0);
}

TEST_F(PpuTest, ScanlineModeTimings) {
    ppu.tick(51); 
    EXPECT_EQ(ppu.read(0xFF44), 1);
    EXPECT_EQ(ppu.read(0xFF41) & 0x03, 2);

    ppu.tick(20);
    EXPECT_EQ(ppu.read(0xFF41) & 0x03, 3);

    ppu.tick(43);
    EXPECT_EQ(ppu.read(0xFF41) & 0x03, 0);
}

TEST_F(PpuTest, VBlankInterruptTriggering) {
    ppu.write(0xFF40, 0x00);
    ppu.write(0xFF40, 0x91);

    for (int i = 0; i < 143; i++)
        ppu.tick(114);

    EXPECT_EQ(ppu.read(0xFF44), 143);
    EXPECT_FALSE(ppu.is_vblank_interrupt_requested());

    ppu.tick(114);
    
    EXPECT_EQ(ppu.read(0xFF44), 144);
    EXPECT_EQ(ppu.read(0xFF41) & 0x03, 1);
    EXPECT_TRUE(ppu.is_vblank_interrupt_requested());
}

TEST_F(PpuTest, LycInterruptTriggering) {
    ppu.write(0xFF40, 0x00);
    ppu.write(0xFF40, 0x91);

    ppu.write(0xFF45, 5);
    ppu.write(0xFF41, 0x40);

    for (int i = 0; i < 4; i++)
        ppu.tick(114);

    EXPECT_EQ(ppu.read(0xFF44), 4);
    EXPECT_FALSE(ppu.is_stat_interrupt_requested());

    ppu.tick(114);

    EXPECT_EQ(ppu.read(0xFF44), 5);
    EXPECT_TRUE((ppu.read(0xFF41) & 0x04) != 0);
    EXPECT_TRUE(ppu.is_stat_interrupt_requested());
}