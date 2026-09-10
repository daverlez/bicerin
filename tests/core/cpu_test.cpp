#include <gtest/gtest.h>
#include <cpu/cpu.h>
#include <mmu/mmu.h>

class CpuTest : public ::testing::Test {
protected:
    Mmu mmu;
    Cpu cpu;

    CpuTest() : cpu(mmu) {}
    void SetUp() override { cpu.reset(); }
};

TEST_F(CpuTest, Reset) {
    Mmu mmu;
    Cpu cpu(mmu);
    cpu.reset();

    Cpu::Registers registers = cpu.get_registers();
    EXPECT_EQ(registers.a, 0x00);
    EXPECT_EQ(registers.b, 0x00);
    EXPECT_EQ(registers.c, 0x00);
    EXPECT_EQ(registers.d, 0x00);
    EXPECT_EQ(registers.e, 0x00);
    EXPECT_EQ(registers.f, 0x00);
    EXPECT_EQ(registers.h, 0x00);
    EXPECT_EQ(registers.l, 0x00);
    EXPECT_EQ(registers.sp, 0x0000);
    EXPECT_EQ(registers.pc, 0x0100);
}

TEST_F(CpuTest, RegisterPairs) {
    Mmu mmu;
    Cpu cpu(mmu);
    cpu.reset();

    Cpu::Registers registers = cpu.get_registers();

    registers.set_af(0x1234);
    registers.set_bc(0x5678);
    registers.set_de(0x9012);
    registers.set_hl(0x3456);

    EXPECT_EQ(registers.a, 0x12);
    EXPECT_EQ(registers.b, 0x56);
    EXPECT_EQ(registers.c, 0x78);
    EXPECT_EQ(registers.d, 0x90);
    EXPECT_EQ(registers.e, 0x12);
    EXPECT_EQ(registers.f, 0x34);
    EXPECT_EQ(registers.h, 0x34);
    EXPECT_EQ(registers.l, 0x56);

    registers.a = 0x01;
    registers.b = 0x02;
    registers.c = 0x03;
    registers.d = 0x04;
    registers.e = 0x05;
    registers.f = 0x06;
    registers.h = 0x07;
    registers.l = 0x08;

    EXPECT_EQ(registers.get_af(), 0x0106);
    EXPECT_EQ(registers.get_bc(), 0x0203);
    EXPECT_EQ(registers.get_de(), 0x0405);
    EXPECT_EQ(registers.get_hl(), 0x0708);
}

TEST_F(CpuTest, Inc_r8_Inc_hl) {
    Mmu mmu;
    Cpu cpu(mmu);
    cpu.reset();

    mmu.write(cpu.get_registers().pc, 0x04);        // INC B
    uint8_t cycles = cpu.tick();

    EXPECT_EQ(cycles, 1);
    EXPECT_EQ(cpu.get_registers().b, 0x01);
    EXPECT_EQ(cpu.get_registers().pc, 0x0101);
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));

    for (int i = 0; i < 14; ++i) {
        mmu.write(cpu.get_registers().pc, 0x04);    // INC B
        cpu.tick();
    }
    EXPECT_EQ(cpu.get_registers().b, 0x0F);

    mmu.write(cpu.get_registers().pc, 0x04);
    cpu.tick();

    EXPECT_EQ(cpu.get_registers().b, 0x10);
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));

    mmu.write(0x0000, 0xFF);
    mmu.write(cpu.get_registers().pc, 0x34);        // INC [HL]

    cycles = cpu.tick();

    EXPECT_EQ(cycles, 3);
    EXPECT_EQ(mmu.read(0x0000), 0x00);

    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
}