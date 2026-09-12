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

TEST_F(CpuTest, Ld_r8_r8) {
    mmu.write(cpu.get_registers().pc, 0x0C);    // INC C
    cpu.tick();
    EXPECT_EQ(cpu.get_registers().c, 0x01);

    mmu.write(cpu.get_registers().pc, 0x41);    // LD B, C
    uint8_t cycles = cpu.tick();

    EXPECT_EQ(cycles, 1);
    EXPECT_EQ(cpu.get_registers().b, 0x01);
}

TEST_F(CpuTest, Ld_r8_hl) {
    mmu.write(0x0000, 0xAA);            // Writing 0xAA in address pointed by [HL]

    mmu.write(cpu.get_registers().pc, 0x46);   // LD B, [HL]
    uint8_t cycles = cpu.tick();

    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(cpu.get_registers().b, 0xAA);
}

TEST_F(CpuTest, Ld_hl_r8) {
    mmu.write(cpu.get_registers().pc, 0x04);    // INC B
    cpu.tick();
    mmu.write(cpu.get_registers().pc, 0x04);    // INC B
    cpu.tick();
    EXPECT_EQ(cpu.get_registers().b, 0x02);

    mmu.write(cpu.get_registers().pc, 0x70);    // LD [HL], B
    uint8_t cycles = cpu.tick();

    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(mmu.read(0x0000), 0x02);
}

TEST_F(CpuTest, Dec_r8_Dec_hl) {
    for(int i=0; i<16; ++i) {
        mmu.write(cpu.get_registers().pc, 0x04);    // INC B
        cpu.tick();
    }
    EXPECT_EQ(cpu.get_registers().b, 0x10);

    mmu.write(cpu.get_registers().pc, 0x05);        // DEC B
    uint8_t cycles = cpu.tick();
    EXPECT_EQ(cycles, 1);
    EXPECT_EQ(cpu.get_registers().b, 0x0F);

    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));

    mmu.write(0x0000, 0x01);
    mmu.write(cpu.get_registers().pc, 0x035);       // DEC [HL]
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 3);
    EXPECT_EQ(mmu.read(0x0000), 0x00);
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
}

TEST_F(CpuTest, Ld_r8_imm8) {
    mmu.write(cpu.get_registers().pc, 0x06);            // LD B, imm8
    mmu.write(cpu.get_registers().pc + 1, 0x42); // imm8 = 0x42
    uint8_t cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(cpu.get_registers().b, 0x42);
    EXPECT_EQ(cpu.get_registers().pc, 0x0102);

    mmu.write(cpu.get_registers().pc, 0x36);            // LD [HL], imm8
    mmu.write(cpu.get_registers().pc + 1, 0x99); // imm8 = 0x99
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 3);
    EXPECT_EQ(mmu.read(0x0000), 0x99);
    EXPECT_EQ(cpu.get_registers().pc, 0x0104);
}

TEST_F(CpuTest, Ld_r16_imm16) {
    mmu.write(cpu.get_registers().pc, 0x01);            // LD BC, 0x1234
    mmu.write(cpu.get_registers().pc + 1, 0x34);
    mmu.write(cpu.get_registers().pc + 2, 0x12);

    uint8_t cycles = cpu.tick();
    EXPECT_EQ(cycles, 3);
    EXPECT_EQ(cpu.get_registers().pc, 0x0103);

    EXPECT_EQ(cpu.get_registers().b, 0x12);
    EXPECT_EQ(cpu.get_registers().c, 0x34);
    EXPECT_EQ(cpu.get_registers().get_bc(), 0x1234);
}

TEST_F(CpuTest, Ld_r16mem_a) {
    uint16_t start_pc = cpu.get_registers().pc;

    std::vector<uint8_t> program = {
        // Test 1: LD [BC], A
        0x3E, 0x42,       // LD A, 0x42
        0x01, 0x00, 0xC0, // LD BC, 0xC000
        0x02,             // LD [BC], A

        // Test 2: LD [DE], A
        0x3E, 0x99,       // LD A, 0x99
        0x11, 0x00, 0xD0, // LD DE, 0xD000
        0x12,             // LD [DE], A

        0x3E, 0xAA,       // LD A, 0xAA
        0x21, 0x00, 0x80, // LD HL, 0x8000
        0x22,             // LD [HL+], A

        // Test 4: LD [HL-], A
        0x3E, 0xBB,       // LD A, 0xBB
        0x32              // LD [HL-], A
    };
    for (size_t i = 0; i < program.size(); ++i) mmu.write(start_pc + i, program[i]);

    // Test 1: LD [BC], A
    cpu.tick();
    cpu.tick();
    uint8_t cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(mmu.read(0xC000), 0x42);

    // Test 2: LD [DE], A
    cpu.tick();
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(mmu.read(0xD000), 0x99);

    // Test 3: LD [HL+], A
    cpu.tick();
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(mmu.read(0x8000), 0xAA);
    EXPECT_EQ(cpu.get_registers().get_hl(), 0x8001);

    // Test 4: LD [HL-], A
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(mmu.read(0x8001), 0xBB);
    EXPECT_EQ(cpu.get_registers().get_hl(), 0x8000);
}

TEST_F(CpuTest, Ld_a_r16mem) {
    uint16_t start_pc = cpu.get_registers().pc;

    mmu.write(0xC000, 0x11); // [BC]
    mmu.write(0xD000, 0x22); // [DE]
    mmu.write(0x8000, 0x33); // [HL+]
    mmu.write(0x8001, 0x44); // [HL-]

    std::vector<uint8_t> program = {
        // Test 1: LD A, [BC]
        0x01, 0x00, 0xC0, // LD BC, 0xC000
        0x0A,             // LD A, [BC]

        // Test 2: LD A, [DE]
        0x11, 0x00, 0xD0, // LD DE, 0xD000
        0x1A,             // LD A, [DE]

        // Test 3: LD A, [HL+]
        0x21, 0x00, 0x80, // LD HL, 0x8000
        0x2A,             // LD A, [HL+]

        // Test 4: LD A, [HL-]
        0x3A              // LD A, [HL-]
    };
    for (size_t i = 0; i < program.size(); ++i) mmu.write(start_pc + i, program[i]);

    // Test 1: LD A, [BC]
    cpu.tick();
    uint8_t cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(cpu.get_registers().a, 0x11);

    // Test 2: LD A, [DE]
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(cpu.get_registers().a, 0x22);

    // Test 3: LD A, [HL+]
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(cpu.get_registers().a, 0x33);
    EXPECT_EQ(cpu.get_registers().get_hl(), 0x8001);

    // Test 4: LD A, [HL-]
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(cpu.get_registers().a, 0x44);
    EXPECT_EQ(cpu.get_registers().get_hl(), 0x8000);
}

TEST_F(CpuTest, Ld_imm16_sp) {
    uint16_t pc = cpu.get_registers().pc;

    // LD SP, 0xAABB
    mmu.write(pc, 0x31);
    mmu.write(pc + 1, 0xBB);
    mmu.write(pc + 2, 0xAA);
    cpu.tick();
    EXPECT_EQ(cpu.get_registers().sp, 0xAABB);

    pc = cpu.get_registers().pc;

    // LD 0xC000, SP
    mmu.write(pc, 0x08);
    mmu.write(pc + 1, 0x00);
    mmu.write(pc + 2, 0xC0);

    uint8_t cycles = cpu.tick();
    EXPECT_EQ(cycles, 5);
    EXPECT_EQ(cpu.get_registers().pc, pc + 3);
    EXPECT_EQ(mmu.read(0xC000), 0xBB);
    EXPECT_EQ(mmu.read(0xC001), 0xAA);
}

TEST_F(CpuTest, Inc_r16) {
    uint16_t start_pc = cpu.get_registers().pc;

    std::vector<uint8_t> program = {
        0x01, 0xFF, 0x00, // LD BC, 0x00FF
        0x03              // INC BC
    };
    for (size_t i = 0; i < program.size(); ++i) mmu.write(start_pc + i, program[i]);

    cpu.tick();
    uint8_t cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(cpu.get_registers().get_bc(), 0x0100);
    EXPECT_EQ(cpu.get_registers().f, 0x00);
}

TEST_F(CpuTest, Dec_r16) {
    uint16_t start_pc = cpu.get_registers().pc;

    std::vector<uint8_t> program = {
        0x11, 0x00, 0x01, // LD DE, 0x0100
        0x1B              // DEC DE
    };
    for (size_t i = 0; i < program.size(); ++i) mmu.write(start_pc + i, program[i]);

    cpu.tick();
    uint8_t cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(cpu.get_registers().get_de(), 0x00FF);
    EXPECT_EQ(cpu.get_registers().f, 0x00);
}

TEST_F(CpuTest, Add_hl_r16) {
    uint16_t start_pc = cpu.get_registers().pc;

    std::vector<uint8_t> program = {
        // Test 1: ADD HL, BC (half-carry, no carry)
        0x21, 0xA2, 0x08, // LD HL, 0x08A2
        0x01, 0x5E, 0x08, // LD BC, 0x085E
        0x09,             // ADD HL, BC

        // Test 2: ADD HL, DE (both half-carry and carry)
        0x11, 0x01, 0x00, // LD DE, 0x0001
        0x21, 0xFF, 0xFF, // LD HL, 0xFFFF
        0x19,             // ADD HL, DE

        // Test 3: ADD HL, HL (carry, no half-carry)
        0x21, 0x00, 0xF0, // LD HL, 0xF000
        0x21, 0x00, 0xF0, // LD HL, 0xF000
        0x29              // ADD HL, HL
    };
    for (size_t i = 0; i < program.size(); ++i) mmu.write(start_pc + i, program[i]);

    // Test 1
    cpu.tick();
    cpu.tick();
    uint8_t cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(cpu.get_registers().get_hl(), 0x1100);
    EXPECT_FALSE(cpu.get_registers().f & Cpu::Registers::Flag::N);
    EXPECT_TRUE(cpu.get_registers().f & Cpu::Registers::Flag::H);
    EXPECT_FALSE(cpu.get_registers().f & Cpu::Registers::Flag::C);

    // Test 2
    cpu.tick();
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(cpu.get_registers().get_hl(), 0x0000);
    EXPECT_FALSE(cpu.get_registers().f & Cpu::Registers::Flag::N);
    EXPECT_TRUE(cpu.get_registers().f & Cpu::Registers::Flag::H);
    EXPECT_TRUE(cpu.get_registers().f & Cpu::Registers::Flag::C);

    // Test 3
    cpu.tick();
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(cpu.get_registers().get_hl(), 0xE000);
    EXPECT_FALSE(cpu.get_registers().f & Cpu::Registers::Flag::N);
    EXPECT_FALSE(cpu.get_registers().f & Cpu::Registers::Flag::H);
    EXPECT_TRUE(cpu.get_registers().f & Cpu::Registers::Flag::C);
}

TEST_F(CpuTest, Jr_imm8) {
    uint16_t pc = cpu.get_registers().pc;

    // JR +5
    mmu.write(pc, 0x18);
    mmu.write(pc + 1, 0x05);
    uint8_t cycles = cpu.tick();
    EXPECT_EQ(cycles, 3);
    EXPECT_EQ(cpu.get_registers().pc, 0x0107);

    // JR -6
    pc = cpu.get_registers().pc;
    mmu.write(pc, 0x18);
    mmu.write(pc + 1, 0xFA);
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 3);
    EXPECT_EQ(cpu.get_registers().pc, 0x0103);
}

TEST_F(CpuTest, Jr_cond_imm8) {
    uint16_t pc = cpu.get_registers().pc;

    // JR NZ, +4
    mmu.write(pc, 0x20);
    mmu.write(pc + 1, 0x04);
    uint8_t cycles = cpu.tick();
    EXPECT_EQ(cycles, 3);
    EXPECT_EQ(cpu.get_registers().pc, 0x0106);

    // JR Z, +50
    pc = cpu.get_registers().pc;
    mmu.write(pc, 0x28);
    mmu.write(pc + 1, 0x50);
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(cpu.get_registers().pc, 0x0108);
}

TEST_F(CpuTest, Add_a_r8_hl) {
    uint16_t start_pc = cpu.get_registers().pc;
    mmu.write(0x8000, 0x01);

    std::vector<uint8_t> program = {
        // Test 1: ADD A, B (Simple addition, no carry/half-carry)
        0x3E, 0x10,       // LD A, 0x10
        0x06, 0x20,       // LD B, 0x20
        0x80,             // ADD A, B

        // Test 2: ADD A, C (Half carry)
        0x3E, 0x0F,       // LD A, 0x0F
        0x0E, 0x01,       // LD C, 0x01
        0x81,             // ADD A, C

        // Test 3: ADD A, D (Zero, Half-carry, Carry)
        0x3E, 0xFF,       // LD A, 0xFF
        0x16, 0x01,       // LD D, 0x01
        0x82,             // ADD A, D

        // Test 4: ADD A, [HL]
        0x3E, 0x01,       // LD A, 0x01
        0x21, 0x00, 0x80, // LD HL, 0x8000
        0x86              // ADD A, [HL]
    };
    for (size_t i = 0; i < program.size(); ++i) mmu.write(start_pc + i, program[i]);

    // Test 1
    cpu.tick();
    cpu.tick();
    uint8_t cycles = cpu.tick();
    EXPECT_EQ(cycles, 1);
    EXPECT_EQ(cpu.get_registers().a, 0x30);
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));

    // Test 2
    cpu.tick();
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 1);
    EXPECT_EQ(cpu.get_registers().a, 0x10);
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));

    // Test 3
    cpu.tick();
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 1);
    EXPECT_EQ(cpu.get_registers().a, 0x00);
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));

    // Test 4
    cpu.tick();
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(cpu.get_registers().a, 0x02);
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));
}

TEST_F(CpuTest, Adc_a_r8_hl) {
    uint16_t start_pc = cpu.get_registers().pc;
    mmu.write(0x8000, 0x00);

    std::vector<uint8_t> program = {
        // Test 1: ADC A, B (Carry = 0)
        0x3E, 0x10,       // LD A, 0x10
        0x06, 0x20,       // LD B, 0x20
        0x88,             // ADC A, B

        // Test 2: ADC A, C (Carry = 1)
        0x3E, 0xFF,       // LD A, 0xFF
        0x87,             // ADD A, A
        0x3E, 0x10,       // LD A, 0x10
        0x0E, 0x20,       // LD C, 0x20
        0x89,             // ADC A, C

        // Test 3: ADC A, D (Carry = 1, Half-carry trigger)
        0x3E, 0xFF,       // LD A, 0xFF
        0x87,             // ADD A, A
        0x3E, 0x0F,       // LD A, 0x0F
        0x16, 0x00,       // LD D, 0x00
        0x8A,             // ADC A, D

        // Test 4: ADC A, [HL] (Carry = 1, Zero trigger, Overflow trigger)
        0x3E, 0xFF,       // LD A, 0xFF
        0x87,             // ADD A, A
        0x3E, 0xFF,       // LD A, 0xFF
        0x21, 0x00, 0x80, // LD HL, 0x8000
        0x8E              // ADC A, [HL]
    };
    for (size_t i = 0; i < program.size(); ++i) mmu.write(start_pc + i, program[i]);

    // Test 1
    cpu.tick();
    cpu.tick();
    uint8_t cycles = cpu.tick();
    EXPECT_EQ(cycles, 1);
    EXPECT_EQ(cpu.get_registers().a, 0x30);
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));

    // Test 2
    cpu.tick();
    cpu.tick();
    cpu.tick();
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 1);
    EXPECT_EQ(cpu.get_registers().a, 0x31);
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));

    // Test 3
    cpu.tick();
    cpu.tick();
    cpu.tick();
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 1);
    EXPECT_EQ(cpu.get_registers().a, 0x10);
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));

    // Test 4
    cpu.tick();
    cpu.tick();
    cpu.tick();
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(cpu.get_registers().a, 0x00);
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));
}

TEST_F(CpuTest, Sub_a_r8_hl) {
    uint16_t start_pc = cpu.get_registers().pc;
    mmu.write(0x8000, 0x05);

    std::vector<uint8_t> program = {
        // Test 1: SUB A, B (Half carry)
        0x3E, 0x10,       // LD A, 0x10
        0x06, 0x01,       // LD B, 0x01
        0x90,             // SUB A, B

        // Test 2: SUB A, C (Carry, Half carry)
        0x3E, 0x10,       // LD A, 0x10
        0x0E, 0x20,       // LD C, 0x20
        0x91,             // SUB A, C

        // Test 3: SUB A, [HL] (Zero flag)
        0x3E, 0x05,       // LD A, 0x05
        0x21, 0x00, 0x80, // LD HL, 0x8000
        0x96              // SUB A, [HL]
    };
    for (size_t i = 0; i < program.size(); ++i) mmu.write(start_pc + i, program[i]);

    // Test 1
    cpu.tick();
    cpu.tick();
    uint8_t cycles = cpu.tick();
    EXPECT_EQ(cycles, 1);
    EXPECT_EQ(cpu.get_registers().a, 0x0F);
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));

    // Test 2
    cpu.tick();
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 1);
    EXPECT_EQ(cpu.get_registers().a, 0xF0);
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));

    // Test 3
    cpu.tick();
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(cpu.get_registers().a, 0x00);
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));
}

TEST_F(CpuTest, Sbc_a_r8_hl) {
    uint16_t start_pc = cpu.get_registers().pc;
    mmu.write(0x8000, 0x05);

    std::vector<uint8_t> program = {
        // Test 1: SBC A, B (Carry = 0)
        0x3E, 0x10,       // LD A, 0x10
        0x06, 0x01,       // LD B, 0x01
        0x98,             // SBC A, B

        // Test 2: SBC A, C (Carry = 1)
        0x3E, 0x00,       // LD A, 0x00
        0x0E, 0x01,       // LD C, 0x01
        0x91,             // SUB A, C
        0x3E, 0x10,       // LD A, 0x10
        0x0E, 0x02,       // LD C, 0x02
        0x99,             // SBC A, C

        // Test 3: SBC A, [HL] (Carry = 1, Overflow wrap)
        0x3E, 0x00,       // LD A, 0x00
        0x16, 0x01,       // LD D, 0x01
        0x92,             // SUB A, D
        0x3E, 0x05,       // LD A, 0x05
        0x21, 0x00, 0x80, // LD HL, 0x8000
        0x9E              // SBC A, [HL]
    };
    for (size_t i = 0; i < program.size(); ++i) mmu.write(start_pc + i, program[i]);

    // Test 1
    cpu.tick();
    cpu.tick();
    uint8_t cycles = cpu.tick();
    EXPECT_EQ(cycles, 1);
    EXPECT_EQ(cpu.get_registers().a, 0x0F);
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));

    // Test 2
    cpu.tick();
    cpu.tick();
    cpu.tick();
    cpu.tick();
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 1);
    EXPECT_EQ(cpu.get_registers().a, 0x0D);
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));

    // Test 3
    cpu.tick();
    cpu.tick();
    cpu.tick();
    cpu.tick();
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(cpu.get_registers().a, 0xFF);
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));
}

TEST_F(CpuTest, And_a_r8_hl) {
    uint16_t start_pc = cpu.get_registers().pc;
    mmu.write(0x8000, 0x55);

    std::vector<uint8_t> program = {
        // Test 1: AND A, B (Non-zero result)
        0x3E, 0xFF,       // LD A, 0xFF
        0x06, 0x0F,       // LD B, 0x0F
        0xA0,             // AND A, B

        // Test 2: AND A, C (Zero result)
        0x3E, 0x10,       // LD A, 0x10
        0x0E, 0x01,       // LD C, 0x01
        0xA1,             // AND A, C

        // Test 3: AND A, [HL]
        0x3E, 0xAA,       // LD A, 0xAA
        0x21, 0x00, 0x80, // LD HL, 0x8000
        0xA6              // AND A, [HL]
    };
    for (size_t i = 0; i < program.size(); ++i) mmu.write(start_pc + i, program[i]);

    // Test 1
    cpu.tick();
    cpu.tick();
    uint8_t cycles = cpu.tick();
    EXPECT_EQ(cycles, 1);
    EXPECT_EQ(cpu.get_registers().a, 0x0F);
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));

    // Test 2
    cpu.tick();
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 1);
    EXPECT_EQ(cpu.get_registers().a, 0x00);
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));

    // Test 3
    cpu.tick();
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(cpu.get_registers().a, 0x00);
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));
}

TEST_F(CpuTest, Xor_a_r8_hl) {
    uint16_t start_pc = cpu.get_registers().pc;
    mmu.write(0x8000, 0x55);

    std::vector<uint8_t> program = {
        // Test 1: XOR A, B (Non-zero result)
        0x3E, 0xFF,       // LD A, 0xFF
        0x06, 0x0F,       // LD B, 0x0F
        0xA8,             // XOR A, B

        // Test 2: XOR A, A (Zero result)
        0x3E, 0x42,       // LD A, 0x42
        0xAF,             // XOR A, A

        // Test 3: XOR A, [HL] (All bits set)
        0x3E, 0xAA,       // LD A, 0xAA
        0x21, 0x00, 0x80, // LD HL, 0x8000
        0xAE              // XOR A, [HL]
    };
    for (size_t i = 0; i < program.size(); ++i) mmu.write(start_pc + i, program[i]);

    // Test 1
    cpu.tick();
    cpu.tick();
    uint8_t cycles = cpu.tick();
    EXPECT_EQ(cycles, 1);
    EXPECT_EQ(cpu.get_registers().a, 0xF0);
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));

    // Test 2
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 1);
    EXPECT_EQ(cpu.get_registers().a, 0x00);
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));

    // Test 3
    cpu.tick();
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(cpu.get_registers().a, 0xFF);
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));
}

TEST_F(CpuTest, Or_a_r8_hl) {
    uint16_t start_pc = cpu.get_registers().pc;
    mmu.write(0x8000, 0x55);

    std::vector<uint8_t> program = {
        // Test 1: OR A, B (Non-zero result)
        0x3E, 0x0A,       // LD A, 0x0A
        0x06, 0x05,       // LD B, 0x05
        0xB0,             // OR A, B

        // Test 2: OR A, C (Zero result)
        0x3E, 0x00,       // LD A, 0x00
        0x0E, 0x00,       // LD C, 0x00
        0xB1,             // OR A, C

        // Test 3: OR A, [HL] (All bits set)
        0x3E, 0xAA,       // LD A, 0xAA
        0x21, 0x00, 0x80, // LD HL, 0x8000
        0xB6              // OR A, [HL]
    };
    for (size_t i = 0; i < program.size(); ++i) mmu.write(start_pc + i, program[i]);

    // Test 1
    cpu.tick();
    cpu.tick();
    uint8_t cycles = cpu.tick();
    EXPECT_EQ(cycles, 1);
    EXPECT_EQ(cpu.get_registers().a, 0x0F);
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));

    // Test 2
    cpu.tick();
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 1);
    EXPECT_EQ(cpu.get_registers().a, 0x00);
    EXPECT_TRUE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));

    // Test 3
    cpu.tick();
    cpu.tick();
    cycles = cpu.tick();
    EXPECT_EQ(cycles, 2);
    EXPECT_EQ(cpu.get_registers().a, 0xFF);
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::Z));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::N));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::H));
    EXPECT_FALSE(cpu.get_registers().get_flag(Cpu::Registers::Flag::C));
}
