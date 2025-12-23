

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