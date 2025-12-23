

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