

#include "nes/cpu.hpp"

#include <gtest/gtest.h>

#include "nes/memory.hpp"

using std::make_shared;
using std::shared_ptr;
using testing::Test;

class CPUTest : public Test {
 protected:
  shared_ptr<Memory> memory;
  shared_ptr<Bus> bus;
  shared_ptr<CPU> cpu;

  void SetUp() override {
    memory = make_shared<Memory>(0x0000, 0xffff);
    bus = make_shared<Bus>();
    bus->add(memory);
    cpu = make_shared<CPU>(bus);
  }
};

TEST_F(CPUTest, SetFlag) {
  // arrange
  auto d = cpu->getFlag(Flags::D);
  // act
  cpu->setFlag(Flags::D);
  // assert
  ASSERT_EQ(d, false);
  d = cpu->getFlag(Flags::D);
  ASSERT_EQ(d, true);
}

TEST_F(CPUTest, ClearFlag) {
  // arrange
  auto i = cpu->getFlag(Flags::I);
  // act
  cpu->clearFlag(Flags::I);
  // assert
  ASSERT_EQ(i, true);
  i = cpu->getFlag(Flags::I);
  ASSERT_EQ(i, false);
}

TEST_F(CPUTest, SetFlagTrue) {
  // arrange
  auto d = cpu->getFlag(Flags::D);
  // act
  cpu->setFlag(Flags::D, true);
  // assert
  ASSERT_EQ(d, false);
  d = cpu->getFlag(Flags::D);
  ASSERT_EQ(d, true);
}

TEST_F(CPUTest, SetFlagFalse) {
  // arrange
  auto i = cpu->getFlag(Flags::I);
  // act
  cpu->setFlag(Flags::I, false);
  // assert
  ASSERT_EQ(i, true);
  i = cpu->getFlag(Flags::I);
  ASSERT_EQ(i, false);
}

TEST_F(CPUTest, SetZero) {
  // arrange
  auto z = cpu->getFlag(Flags::Z);
  // act
  cpu->setZN(0x00);
  // assert
  ASSERT_EQ(z, false);
  z = cpu->getFlag(Flags::Z);
  ASSERT_EQ(z, true);
}

TEST_F(CPUTest, SetNegative) {
  // arrange
  auto n = cpu->getFlag(Flags::N);
  // act
  cpu->setZN(0xf0);
  // assert
  ASSERT_EQ(n, false);
  n = cpu->getFlag(Flags::N);
  ASSERT_EQ(n, true);
}

TEST_F(CPUTest, Push8) {
  // arrange
  auto sp = cpu->sp;
  // act
  cpu->push8(0xf0);
  // assert
  ASSERT_EQ(cpu->sp, sp - 1);
  auto value = bus->read8(STACK_PAGE + sp);
  ASSERT_EQ(value, 0xf0);
}

TEST_F(CPUTest, Push16) {
  // arrange
  auto sp = cpu->sp;
  // act
  cpu->push16(0xf011);
  // assert
  ASSERT_EQ(cpu->sp, sp - 2);
  auto value = bus->read16(STACK_PAGE + sp - 1);
  ASSERT_EQ(value, 0xf011);
}

TEST_F(CPUTest, Pop8) {
  // arrange
  auto sp = cpu->sp;
  // act
  cpu->push8(0xf0);
  // assert
  ASSERT_EQ(cpu->sp, sp - 1);
  auto value = cpu->pop8();
  ASSERT_EQ(value, 0xf0);
}

TEST_F(CPUTest, Pop16) {
  // arrange
  auto sp = cpu->sp;
  // act
  cpu->push16(0xf011);
  // assert
  ASSERT_EQ(cpu->sp, sp - 2);
  auto value = cpu->pop16();
  ASSERT_EQ(value, 0xf011);
}

TEST_F(CPUTest, AbsAddressing) {
  // arrange
  cpu->pc = 0x02000;
  uint8_t code[] = {0xad, 0x23, 0x45};
  memory->set(0x02000, code, 3);
  // act
  cpu->abs();
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Abs);
  ASSERT_EQ(cpu->address, 0x4523);
}

TEST_F(CPUTest, AbsxAddressingNoPenality) {
  // arrange
  cpu->pc = 0x02000;
  cpu->x = 0xf9;
  uint8_t code[] = {0xbd, 0x00, 0x45};
  memory->set(0x02000, code, 3);
  // act
  cpu->absx();
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Absx);
  ASSERT_EQ(cpu->address, 0x45f9);
  ASSERT_EQ(cpu->penality, false);
}

TEST_F(CPUTest, AbsxAddressingPenality) {
  // arrange
  cpu->pc = 0x02000;
  cpu->x = 0xf9;
  uint8_t code[] = {0xbd, 0x07, 0x45};
  memory->set(0x02000, code, 3);
  // act
  cpu->absx();
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Absx);
  ASSERT_EQ(cpu->address, 0x4600);
  ASSERT_EQ(cpu->penality, true);
}

TEST_F(CPUTest, AbsyAddressingNoPenality) {
  // arrange
  cpu->pc = 0x02000;
  cpu->y = 0xf9;
  uint8_t code[] = {0xb9, 0x00, 0x45};
  memory->set(0x02000, code, 3);
  // act
  cpu->absy();
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Absy);
  ASSERT_EQ(cpu->address, 0x45f9);
  ASSERT_EQ(cpu->penality, false);
}

TEST_F(CPUTest, AbsyAddressingPenality) {
  // arrange
  cpu->pc = 0x02000;
  cpu->y = 0xf9;
  uint8_t code[] = {0xb9, 0x07, 0x45};
  memory->set(0x02000, code, 3);
  // act
  cpu->absy();
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Absy);
  ASSERT_EQ(cpu->address, 0x4600);
  ASSERT_EQ(cpu->penality, true);
}

TEST_F(CPUTest, AccAddressing) {
  // arrange
  cpu->pc = 0x02000;
  cpu->a = 0xf9;
  // act
  cpu->acc();
  auto value = cpu->read8();
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Acc);
  ASSERT_EQ(value, cpu->a);
}

TEST_F(CPUTest, ImmAddressing) {
  // arrange
  cpu->pc = 0x02000;
  uint8_t code[] = {0xa9, 0x87};
  memory->set(0x02000, code, 2);
  // act
  cpu->imm();
  auto value = cpu->read8();
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Imm);
  ASSERT_EQ(cpu->address, cpu->pc + 1);
  ASSERT_EQ(value, 0x87);
}

TEST_F(CPUTest, ImpAddressing) {
  // arrange
  cpu->pc = 0x02000;
  uint8_t code[] = {0xea};
  memory->set(0x02000, code, 1);
  // act
  cpu->imp();
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Imp);
}

TEST_F(CPUTest, IndAddressing) {
  // arrange
  cpu->pc = 0x02000;
  uint8_t code[] = {0x6c, 0x12, 0x30};
  memory->set(0x02000, code, 3);
  uint8_t addr[] = {0x00, 0x45};
  memory->set(0x3012, addr, 2);

  // act
  cpu->ind();
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Ind);
  ASSERT_EQ(cpu->address, 0x4500);
}

TEST_F(CPUTest, IndxAddressing) {
  // arrange
  cpu->pc = 0x02000;
  cpu->x = 0x45;
  uint8_t code[] = {0x61, 0x12};
  memory->set(0x02000, code, 2);
  uint8_t addr[] = {0x00, 0x45};
  memory->set(0x57, addr, 2);

  // act
  cpu->indx();
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Indx);
  ASSERT_EQ(cpu->address, 0x4500);
}

TEST_F(CPUTest, IndyAddressingNoPenality) {
  // arrange
  cpu->pc = 0x02000;
  cpu->y = 0x45;
  uint8_t code[] = {0x71, 0x12};
  memory->set(0x02000, code, 2);
  uint8_t addr[] = {0x00, 0x45};
  memory->set(0x12, addr, 2);

  // act
  cpu->indy();
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Indy);
  ASSERT_EQ(cpu->address, 0x4545);
  ASSERT_EQ(cpu->penality, false);
}

TEST_F(CPUTest, IndyAddressingPenality) {
  // arrange
  cpu->pc = 0x02000;
  cpu->y = 0x45;
  uint8_t code[] = {0x71, 0x12};
  memory->set(0x02000, code, 2);
  uint8_t addr[] = {0xf0, 0x45};
  memory->set(0x12, addr, 2);

  // act
  cpu->indy();
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Indy);
  ASSERT_EQ(cpu->address, 0x4635);
  ASSERT_EQ(cpu->penality, true);
}

TEST_F(CPUTest, RelAddressingPositive) {
  // arrange
  cpu->pc = 0x02000;
  uint8_t code[] = {0x30, 0x50};
  memory->set(0x02000, code, 2);
  uint8_t addr[] = {0xf0, 0x45};
  memory->set(0x12, addr, 2);

  // act
  cpu->rel();
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Rel);
  ASSERT_EQ(cpu->address, cpu->pc + 0x50);
}

TEST_F(CPUTest, RelAddressingNegative) {
  // arrange
  cpu->pc = 0x02000;
  uint8_t code[] = {0x30, 0xf0};
  memory->set(0x02000, code, 2);
  uint8_t addr[] = {0xf0, 0x45};
  memory->set(0x12, addr, 2);

  // act
  cpu->rel();
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Rel);
  ASSERT_EQ(cpu->address, cpu->pc + 0x100 - 0xf0);
}

TEST_F(CPUTest, ZpAddressing) {
  // arrange
  cpu->pc = 0x02000;
  uint8_t code[] = {0x65, 0xf0};
  memory->set(0x02000, code, 2);
  uint8_t values[] = {0x45};
  memory->set(0xf0, values, 1);

  // act
  cpu->zp();
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Zp);
  ASSERT_EQ(cpu->address, 0xf0);
  ASSERT_EQ(cpu->read8(), 0x45);
}

TEST_F(CPUTest, ZpxAddressing) {
  // arrange
  cpu->pc = 0x02000;
  cpu->x = 0x30;
  uint8_t code[] = {0x75, 0x45};
  memory->set(0x02000, code, 2);
  uint8_t values[] = {0x80};
  memory->set(0x75, values, 1);

  // act
  cpu->zpx();
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Zpx);
  ASSERT_EQ(cpu->address, 0x75);
  ASSERT_EQ(cpu->read8(), 0x80);
}

TEST_F(CPUTest, ZpyAddressing) {
  // arrange
  cpu->pc = 0x02000;
  cpu->y = 0x45;
  uint8_t code[] = {0xb6, 0x45};
  memory->set(0x02000, code, 2);
  uint8_t values[] = {0x8f};
  memory->set(0x8a, values, 1);

  // act
  cpu->zpy();
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Zpy);
  ASSERT_EQ(cpu->address, 0x8a);
  ASSERT_EQ(cpu->read8(), 0x8f);
}

TEST_F(CPUTest, AdcImmOverflowPositive) {
  // arrange
  cpu->pc = 0x02000;
  cpu->a = 0x7f;
  cpu->setFlag(Flags::C, true);
  uint8_t code[] = {0x69, 0x7f};
  memory->set(0x02000, code, 2);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Imm);
  ASSERT_EQ(cpu->pc, 0x02002);
  ASSERT_EQ(cpu->a, 0xff);
  ASSERT_EQ(cpu->getFlag(Flags::C), false);
  ASSERT_EQ(cpu->getFlag(Flags::V), true);
  ASSERT_EQ(cpu->getFlag(Flags::N), true);
  ASSERT_EQ(cpu->getFlag(Flags::Z), false);
}

TEST_F(CPUTest, AdcImmUnderflowNegative) {
  // arrange
  cpu->pc = 0x02000;
  cpu->a = 0xf5;
  cpu->setFlag(Flags::C, false);
  uint8_t code[] = {0x69, 0x83};
  memory->set(0x02000, code, 2);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Imm);
  ASSERT_EQ(cpu->pc, 0x02002);
  ASSERT_EQ(cpu->a, 0x78);
  ASSERT_EQ(cpu->getFlag(Flags::C), true);
  ASSERT_EQ(cpu->getFlag(Flags::V), true);
  ASSERT_EQ(cpu->getFlag(Flags::N), false);
  ASSERT_EQ(cpu->getFlag(Flags::Z), false);
}

TEST_F(CPUTest, AndZp) {
  // arrange
  cpu->pc = 0x02000;
  cpu->a = 0xf5;
  uint8_t code[] = {0x25, 0x83};
  memory->set(0x02000, code, 2);
  uint8_t data[] = {0xf0};
  memory->set(0x83, data, 1);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Zp);
  ASSERT_EQ(cpu->pc, 0x02002);
  ASSERT_EQ(cpu->a, 0xf0);
  ASSERT_EQ(cpu->getFlag(Flags::N), true);
  ASSERT_EQ(cpu->getFlag(Flags::Z), false);
}

TEST_F(CPUTest, AslAcc) {
  // arrange
  cpu->pc = 0x02000;
  cpu->a = 0x80;
  uint8_t code[] = {0x0a};
  memory->set(0x02000, code, 1);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Acc);
  ASSERT_EQ(cpu->pc, 0x02001);
  ASSERT_EQ(cpu->a, 0x00);
  ASSERT_EQ(cpu->getFlag(Flags::C), true);
  ASSERT_EQ(cpu->getFlag(Flags::N), false);
  ASSERT_EQ(cpu->getFlag(Flags::Z), true);
}

TEST_F(CPUTest, AslAbs) {
  // arrange
  cpu->pc = 0x02000;
  uint8_t code[] = {0x0e, 0x40, 0x30};
  memory->set(0x02000, code, 3);
  uint8_t data[] = {0x40};
  memory->set(0x3040, data, 1);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Abs);
  ASSERT_EQ(cpu->pc, 0x02003);
  ASSERT_EQ(bus->read8(0x3040), 0x80);
  ASSERT_EQ(cpu->getFlag(Flags::C), false);
  ASSERT_EQ(cpu->getFlag(Flags::N), true);
  ASSERT_EQ(cpu->getFlag(Flags::Z), false);
}

TEST_F(CPUTest, BccFailure) {
  // arrange
  cpu->pc = 0x02000;
  cpu->setFlag(Flags::C, true);
  uint8_t code[] = {0x90, 0x40};
  memory->set(0x02000, code, 2);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Rel);
  ASSERT_EQ(cpu->pc, 0x02002);
}

TEST_F(CPUTest, BccSuccess) {
  // arrange
  cpu->pc = 0x02000;
  cpu->setFlag(Flags::C, false);
  uint8_t code[] = {0x90, 0x40};
  memory->set(0x02000, code, 2);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Rel);
  ASSERT_EQ(cpu->pc, 0x02042);
}

TEST_F(CPUTest, BcsFailure) {
  // arrange
  cpu->pc = 0x02000;
  cpu->setFlag(Flags::C, false);
  uint8_t code[] = {0xb0, 0x40};
  memory->set(0x02000, code, 2);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Rel);
  ASSERT_EQ(cpu->pc, 0x02002);
}

TEST_F(CPUTest, BcsSuccess) {
  // arrange
  cpu->pc = 0x02000;
  cpu->setFlag(Flags::C, true);
  uint8_t code[] = {0xb0, 0x40};
  memory->set(0x02000, code, 2);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Rel);
  ASSERT_EQ(cpu->pc, 0x02042);
}

TEST_F(CPUTest, BeqFailure) {
  // arrange
  cpu->pc = 0x02000;
  cpu->setFlag(Flags::Z, false);
  uint8_t code[] = {0xf0, 0x40};
  memory->set(0x02000, code, 2);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Rel);
  ASSERT_EQ(cpu->pc, 0x02002);
}

TEST_F(CPUTest, BeqSuccess) {
  // arrange
  cpu->pc = 0x02000;
  cpu->setFlag(Flags::Z, true);
  uint8_t code[] = {0xf0, 0x40};
  memory->set(0x02000, code, 2);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Rel);
  ASSERT_EQ(cpu->pc, 0x02042);
}

TEST_F(CPUTest, BitZp) {
  // arrange
  cpu->pc = 0x02000;
  cpu->a = 0xf0;
  uint8_t code[] = {0x24, 0x40};
  memory->set(0x02000, code, 2);
  uint8_t data[] = {0xe0};
  memory->set(0x0040, data, 1);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Zp);
  ASSERT_EQ(cpu->pc, 0x02002);
  ASSERT_EQ(cpu->getFlag(Flags::V), true);
  ASSERT_EQ(cpu->getFlag(Flags::N), true);
  ASSERT_EQ(cpu->getFlag(Flags::Z), false);
}

TEST_F(CPUTest, BmiFailure) {
  // arrange
  cpu->pc = 0x02000;
  cpu->setFlag(Flags::N, false);
  uint8_t code[] = {0x30, 0x40};
  memory->set(0x02000, code, 2);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Rel);
  ASSERT_EQ(cpu->pc, 0x02002);
}

TEST_F(CPUTest, BmiSuccess) {
  // arrange
  cpu->pc = 0x02000;
  cpu->setFlag(Flags::N, true);
  uint8_t code[] = {0x30, 0x40};
  memory->set(0x02000, code, 2);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Rel);
  ASSERT_EQ(cpu->pc, 0x02042);
}

TEST_F(CPUTest, BneFailure) {
  // arrange
  cpu->pc = 0x02000;
  cpu->setFlag(Flags::Z, true);
  uint8_t code[] = {0xd0, 0x40};
  memory->set(0x02000, code, 2);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Rel);
  ASSERT_EQ(cpu->pc, 0x02002);
}

TEST_F(CPUTest, BneSuccess) {
  // arrange
  cpu->pc = 0x02000;
  cpu->setFlag(Flags::Z, false);
  uint8_t code[] = {0xd0, 0x40};
  memory->set(0x02000, code, 2);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Rel);
  ASSERT_EQ(cpu->pc, 0x02042);
}

TEST_F(CPUTest, BplFailure) {
  // arrange
  cpu->pc = 0x02000;
  cpu->setFlag(Flags::N, true);
  uint8_t code[] = {0x10, 0x40};
  memory->set(0x02000, code, 2);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Rel);
  ASSERT_EQ(cpu->pc, 0x02002);
}

TEST_F(CPUTest, BplSuccess) {
  // arrange
  cpu->pc = 0x02000;
  cpu->setFlag(Flags::N, false);
  uint8_t code[] = {0x10, 0x40};
  memory->set(0x02000, code, 2);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Rel);
  ASSERT_EQ(cpu->pc, 0x02042);
}

TEST_F(CPUTest, BrkSuccess) {
  // arrange
  cpu->pc = 0x02000;
  uint8_t code[] = {0x00, 0x40};
  memory->set(0x02000, code, 2);
  bus->write16(IRQ_PROC_ADDR, 0x2345);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Imm);
  ASSERT_EQ(cpu->pc, 0x2345);
  ASSERT_EQ(cpu->getFlag(Flags::I), true);
  auto status = cpu->pop8();
  ASSERT_EQ((status & 0x10) != 0, true);
  ASSERT_EQ((status & 0x20) != 0, true);
  ASSERT_EQ(cpu->pop16(), 0x2002);
}

TEST_F(CPUTest, BvcFailure) {
  // arrange
  cpu->pc = 0x02000;
  cpu->setFlag(Flags::V, true);
  uint8_t code[] = {0x50, 0x40};
  memory->set(0x02000, code, 2);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Rel);
  ASSERT_EQ(cpu->pc, 0x02002);
}

TEST_F(CPUTest, BvcSuccess) {
  // arrange
  cpu->pc = 0x02000;
  cpu->setFlag(Flags::V, false);
  uint8_t code[] = {0x50, 0x40};
  memory->set(0x02000, code, 2);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Rel);
  ASSERT_EQ(cpu->pc, 0x02042);
}

TEST_F(CPUTest, BvsFailure) {
  // arrange
  cpu->pc = 0x02000;
  cpu->setFlag(Flags::V, false);
  uint8_t code[] = {0x70, 0x40};
  memory->set(0x02000, code, 2);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Rel);
  ASSERT_EQ(cpu->pc, 0x02002);
}

TEST_F(CPUTest, BvsSuccess) {
  // arrange
  cpu->pc = 0x02000;
  cpu->setFlag(Flags::V, true);
  uint8_t code[] = {0x70, 0x40};
  memory->set(0x02000, code, 2);

  // act
  cpu->clock(true);
  // assert
  ASSERT_EQ(cpu->addressing, Addressing::Rel);
  ASSERT_EQ(cpu->pc, 0x02042);
}