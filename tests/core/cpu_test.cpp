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

TEST(CpuArithmeticTest, AddToAccumulator) {
    Cpu cpu;
    Bus bus;

    // Simple addition
    cpu.reset();
    cpu.a = 0x05;
    cpu.b = 0x03;

    cpu.pc = 0x0000;
    bus.write(0x0000, 0x80);
    cpu.step(bus);

    EXPECT_EQ(cpu.a, 0x08);
    EXPECT_EQ(cpu.f, 0x00);

    // Addition with half-carry
    cpu.reset();
    cpu.a = 0x0F;
    cpu.c = 0x01;

    cpu.pc = 0x0000;
    bus.write(0x0000, 0x81);
    cpu.step(bus);

    EXPECT_EQ(cpu.a, 0x10);
    EXPECT_EQ(cpu.f, 0x20);

    // Addition with zero flag and carry flag
    cpu.reset();
    cpu.a = 0xFF;
    cpu.d = 0x01;

    cpu.pc = 0x0000;
    bus.write(0x0000, 0x82);
    cpu.step(bus);

    EXPECT_EQ(cpu.a, 0x00);
    EXPECT_EQ(cpu.f, 0xB0);
}

TEST(CpuArithmeticTest, SubAndCompare) {
    Cpu cpu;
    Bus bus;

    // Subtraction (with half-carry)
    cpu.reset();
    cpu.a = 0x10;
    cpu.b = 0x05;

    cpu.pc = 0x0000;
    bus.write(0x0000, 0x90);
    cpu.step(bus);

    EXPECT_EQ(cpu.a, 0x0B);
    EXPECT_EQ(cpu.f, 0x60);

    // Compare
    cpu.reset();
    cpu.a = 0x42;
    cpu.c = 0x42;

    cpu.pc = 0x0000;
    bus.write(0x0000, 0xB9);
    cpu.step(bus);

    EXPECT_EQ(cpu.a, 0x42);
    EXPECT_EQ(cpu.f, 0xC0);

    // Compare (with half-carry)
    cpu.reset();
    cpu.a = 0x05;
    cpu.d = 0x10;

    cpu.pc = 0x0000;
    bus.write(0x0000, 0xBA);
    cpu.step(bus);

    EXPECT_EQ(cpu.a, 0x05);
    EXPECT_EQ(cpu.f, 0x50);
}

TEST(CpuArithmeticTest, AddWithCarry) {
    Cpu cpu;
    Bus bus;

    cpu.reset();
    cpu.pc = 0x0000;

    cpu.a = 0x10;
    cpu.b = 0x20;
    cpu.f = 0x10;

    // ADC A, B
    bus.write(0x0000, 0x88);
    cpu.step(bus);

    EXPECT_EQ(cpu.a, 0x31);
    EXPECT_EQ(cpu.f, 0x00);
}

TEST(CpuArithmeticTest, SubtractWithCarry) {
    Cpu cpu;
    Bus bus;

    cpu.reset();
    cpu.pc = 0x0000;

    cpu.a = 0x10;
    cpu.c = 0x05;
    cpu.f = 0x10;

    // SBC A, C
    bus.write(0x0000, 0x99);
    cpu.step(bus);

    EXPECT_EQ(cpu.a, 0x0A);
    EXPECT_EQ(cpu.f, 0x60);
}

TEST(CpuLogicTest, LogicalAnd) {
    Cpu cpu;
    Bus bus;

    cpu.reset();
    cpu.pc = 0x0000;

    cpu.a = 0xFF;
    cpu.d = 0x0F;

    // AND A, D
    bus.write(0x0000, 0xA2);
    cpu.step(bus);

    EXPECT_EQ(cpu.a, 0x0F);
    EXPECT_EQ(cpu.f, 0x20);
}

TEST(CpuLogicTest, LogicalXorAndZeroFlag) {
    Cpu cpu;
    Bus bus;

    cpu.reset();
    cpu.pc = 0x0000;

    cpu.a = 0xAA;
    cpu.e = 0xAA;

    // XOR A, E
    bus.write(0x0000, 0xAB);
    cpu.step(bus);

    EXPECT_EQ(cpu.a, 0x00);
    EXPECT_EQ(cpu.f, 0x80);
}

TEST(CpuLogicTest, LogicalOr) {
    Cpu cpu;
    Bus bus;

    cpu.reset();
    cpu.pc = 0x0000;

    cpu.a = 0x55;
    cpu.h = 0xAA;

    // OR A, H
    bus.write(0x0000, 0xB4);
    cpu.step(bus);

    EXPECT_EQ(cpu.a, 0xFF);
    EXPECT_EQ(cpu.f, 0x00);
}

TEST(CpuStackTest, PushAndPopRegisters) {
    Cpu cpu;
    Bus bus;

    cpu.reset();
    cpu.pc = 0x0000;
    cpu.sp = 0xFFFE;

    cpu.set_bc(0x1234);
    cpu.set_de(0x0000);

    bus.write(0x0000, 0xC5);    // PUSH BC
    bus.write(0x0001, 0xD1);    // POP DE

    cpu.step(bus);

    EXPECT_EQ(cpu.sp, 0xFFFC);
    EXPECT_EQ(bus.read(0xFFFD), 0x12);
    EXPECT_EQ(bus.read(0xFFFC), 0x34);

    cpu.step(bus);

    EXPECT_EQ(cpu.sp, 0xFFFE);
    EXPECT_EQ(cpu.get_de(), 0x1234);
}

TEST(CpuControlFlowTest, CallAndReturn) {
    Cpu cpu;
    Bus bus;

    cpu.reset();
    cpu.pc = 0x0100;
    cpu.sp = 0xFFFE;

    // CALL 0x0500
    bus.write(0x0100, 0xCD);    // CALL imm16
    bus.write(0x0101, 0x00);    // lo
    bus.write(0x0102, 0x05);    // hi

    bus.write(0x0103, 0x3E);    // LD A, d8
    bus.write(0x0104, 0x99);

    bus.write(0x0500, 0xC9);    // RET

    cpu.step(bus);

    EXPECT_EQ(cpu.pc, 0x0500);
    EXPECT_EQ(cpu.sp, 0xFFFC);
    EXPECT_EQ(bus.read(0xFFFD), 0x01);
    EXPECT_EQ(bus.read(0xFFFC), 0x03);

    cpu.step(bus);

    EXPECT_EQ(cpu.pc, 0x0103);
    EXPECT_EQ(cpu.sp, 0xFFFE);

    cpu.step(bus);
    EXPECT_EQ(cpu.a, 0x99);
}

TEST(CpuMemoryTest, LoadHighRamInstructions) {
    Cpu cpu;
    Bus bus;

    cpu.reset();
    cpu.pc = 0x0000;

    // LDH [imm8], A
    cpu.a = 0x42;
    bus.write(0x0000, 0xE0);
    bus.write(0x0001, 0x40);

    cpu.step(bus);

    EXPECT_EQ(bus.read(0xFF40), 0x42);
    EXPECT_EQ(cpu.pc, 0x0002);

    // LDH A, [C]
    cpu.reset();
    cpu.pc = 0x0000;
    cpu.c = 0x44;
    bus.write(0xFF44, 0x99);
    bus.write(0x0000, 0xF2);

    cpu.step(bus);

    EXPECT_EQ(cpu.a, 0x99);
    EXPECT_EQ(cpu.pc, 0x0001);
}

TEST(CpuBitwiseTest, TestBitInstruction) {
    Cpu cpu;
    Bus bus;

    cpu.reset();
    cpu.pc = 0x0000;
    cpu.h = 0x80;

    // Bit 7 of H must be 1.
    bus.write(0x0000, 0xCB);    // CB
    bus.write(0x0001, 0x7C);    // BIT 7, H

    cpu.step(bus);

    EXPECT_FALSE(cpu.f & 0x80);
    EXPECT_TRUE(cpu.f & 0x20);
    EXPECT_EQ(cpu.pc, 0x0002);

    // Bit 0 of H must be 0.
    cpu.pc = 0x0000;

    bus.write(0x0000, 0xCB);    // CB
    bus.write(0x0001, 0x44);    // BIT 0, H

    cpu.step(bus);

    EXPECT_TRUE(cpu.f & 0x80);
    EXPECT_TRUE(cpu.f & 0x20);
}

TEST(CpuBitwiseTest, SetAndResetInstructions) {
    Cpu cpu;
    Bus bus;

    cpu.reset();
    cpu.pc = 0x0000;
    cpu.b = 0x00;

    bus.write(0x0000, 0xCB);    // CB
    bus.write(0x0001, 0xF8);    // SET 7, B
    cpu.step(bus);

    EXPECT_EQ(cpu.b, 0x80);
    EXPECT_EQ(cpu.pc, 0x0002);

    cpu.reset();
    cpu.pc = 0x0000;
    cpu.a = 0xFF;

    bus.write(0x0000, 0xCB);    // CB
    bus.write(0x0001, 0x9F);    // RES 3, A
    cpu.step(bus);

    EXPECT_EQ(cpu.a, 0xF7);

    cpu.reset();
    cpu.pc = 0x0000;
    cpu.set_hl(0xC000);
    bus.write(0xC000, 0x00);

    bus.write(0x0000, 0xCB);    // CB
    bus.write(0x0001, 0xC6);    // SET 0, [HL]
    cpu.step(bus);

    EXPECT_EQ(bus.read(0xC000), 0x01);
}

TEST(CpuBitwiseTest, RotationsAndShifts) {
    Cpu cpu;
    Bus bus;

    cpu.reset();
    cpu.pc = 0x0000;
    cpu.a = 0xA5; // 1010 0101

    bus.write(0x0000, 0xCB);    // CB
    bus.write(0x0001, 0x37);    // SWAP A
    cpu.step(bus);

    EXPECT_EQ(cpu.a, 0x5A); // 0101 1010
    EXPECT_EQ(cpu.f, 0x00);

    cpu.reset();
    cpu.pc = 0x0000;
    cpu.c = 0x80; // 1000 0000
    cpu.f = 0x10; // C = 1

    bus.write(0x0000, 0xCB);    // CB
    bus.write(0x0001, 0x11);    // RL C
    cpu.step(bus);

    EXPECT_EQ(cpu.c, 0x01); // 0000 0001
    EXPECT_EQ(cpu.f, 0x10);

    bus.write(0x0002, 0xCB);
    bus.write(0x0003, 0x11);
    cpu.step(bus);
    EXPECT_EQ(cpu.c, 0x03); // 0000 0011
    EXPECT_EQ(cpu.f, 0x00);
}

TEST(CpuStackMathTest, StackPointerOffset) {
    Cpu cpu;
    Bus bus;

    // ADD SP, 10
    cpu.reset();
    cpu.pc = 0x0000;
    cpu.sp = 0x1000;

    bus.write(0x0000, 0xE8);
    bus.write(0x0001, 0x0A);
    cpu.step(bus);

    EXPECT_EQ(cpu.sp, 0x100A);
    EXPECT_EQ(cpu.f, 0x00);

    // LD HL, SP - 5
    cpu.reset();
    cpu.pc = 0x0000;
    cpu.sp = 0x1000;

    bus.write(0x0000, 0xF8);
    bus.write(0x0001, 0xFB);
    cpu.step(bus);

    EXPECT_EQ(cpu.get_hl(), 0x0FFB);
    EXPECT_EQ(cpu.sp, 0x1000);
    EXPECT_EQ(cpu.f, 0x00);
}