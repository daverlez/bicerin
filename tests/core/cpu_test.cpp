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

TEST(CpuIntegrationTest, ExecuteAllLoadImmediate8Bit) {
    Cpu cpu;
    Bus bus;

    cpu.reset();
    cpu.pc = 0x0000;

    // LD B, 0x11
    bus.write(0x0000, 0x06); bus.write(0x0001, 0x11);
    // LD C, 0x22
    bus.write(0x0002, 0x0E); bus.write(0x0003, 0x22);
    // LD D, 0x33
    bus.write(0x0004, 0x16); bus.write(0x0005, 0x33);
    // LD E, 0x44 (Opcode: 0x1E)
    bus.write(0x0006, 0x1E); bus.write(0x0007, 0x44);
    // LD H, 0x80 (Opcode: 0x26)
    bus.write(0x0008, 0x26); bus.write(0x0009, 0x80);
    // LD L, 0x00 (Opcode: 0x2E)
    bus.write(0x000A, 0x2E); bus.write(0x000B, 0x00);
    // LD [HL], 0x55 (Opcode: 0x36)
    bus.write(0x000C, 0x36); bus.write(0x000D, 0x55);
    // LD A, 0x66 (Opcode: 0x3E)
    bus.write(0x000E, 0x3E); bus.write(0x000F, 0x66);

    for (int i = 0; i < 8; ++i)
        cpu.step(bus);

    EXPECT_EQ(cpu.b, 0x11);
    EXPECT_EQ(cpu.c, 0x22);
    EXPECT_EQ(cpu.d, 0x33);
    EXPECT_EQ(cpu.e, 0x44);
    EXPECT_EQ(cpu.a, 0x66);

    EXPECT_EQ(cpu.h, 0x80);
    EXPECT_EQ(cpu.l, 0x00);
    EXPECT_EQ(cpu.get_hl(), 0x8000);

    EXPECT_EQ(bus.read(0x8000), 0x55);
    EXPECT_EQ(cpu.pc, 0x0010);
}

TEST(CpuIntegrationTest, ExecuteBlock1RegisterTransfers) {
    Cpu cpu;
    Bus bus;

    cpu.reset();
    cpu.pc = 0x0000;

    cpu.a = 0x99;
    cpu.c = 0x42;
    cpu.set_hl(0x8000);

    bus.write(0x0000, 0x47); // LD B, A
    bus.write(0x0001, 0x71); // LD [HL], C

    cpu.step(bus);
    cpu.step(bus);

    EXPECT_EQ(cpu.b, cpu.a);
    EXPECT_EQ(bus.read(0x8000), cpu.c);
    EXPECT_EQ(cpu.pc, 0x0002);
}