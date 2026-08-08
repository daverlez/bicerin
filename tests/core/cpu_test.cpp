#include <gtest/gtest.h>
#include <cpu/cpu.h>

TEST(CpuTest, Reset) {
    Cpu cpu;
    cpu.reset();

    Cpu::Registers registers = cpu.get_registers();
    EXPECT_EQ(registers.a, 0x00);
    EXPECT_EQ(registers.b, 0x00);
    EXPECT_EQ(registers.c, 0x00);
    EXPECT_EQ(registers.d, 0x00);
    EXPECT_EQ(registers.e, 0x00);
    EXPECT_EQ(registers.h, 0x00);
    EXPECT_EQ(registers.l, 0x00);
    EXPECT_EQ(registers.sp, 0x0000);
    EXPECT_EQ(registers.pc, 0x0100);
}