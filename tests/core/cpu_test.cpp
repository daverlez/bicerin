#include <gtest/gtest.h>
#include "core/cpu.h"

TEST(CpuTest, InitialStateAfterReset) {
    Cpu cpu;
    cpu.reset();

    EXPECT_EQ(cpu.a,        0x00);
    EXPECT_EQ(cpu.f,        0x00);
    EXPECT_EQ(cpu.get_bc(), 0x00);
    EXPECT_EQ(cpu.pc,       0x0100);
}

TEST(CpuTest, RegisterPairing) {
    Cpu cpu;
    cpu.set_bc(0x1234);

    EXPECT_EQ(cpu.b,        0x12);
    EXPECT_EQ(cpu.c,        0x34);
    EXPECT_EQ(cpu.get_bc(), 0x1234);
}