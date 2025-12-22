#include "nes/bus.hpp"

#include <gtest/gtest.h>

#include "nes/memory.hpp"

using std::make_shared;
using std::shared_ptr;
using testing::Test;

class BusTest : public Test {
 protected:
  shared_ptr<Memory> memory = nullptr;
  shared_ptr<Bus> bus = nullptr;

 protected:
  void SetUp() override {
    memory = make_shared<Memory>(0x0000, 0x07ff);
    bus = make_shared<Bus>();
    bus->add(memory);
  }

  void TearDown() override {
    bus.reset();
    memory.reset();
  }
};

TEST_F(BusTest, ReadWrite8Success) {
  // arrange
  uint8_t value = 0x56;
  uint16_t addr = 0x0240;

  // act
  bus->write8(addr, value);
  auto result = bus->read8(addr);

  // assert
  ASSERT_EQ(result, value);
}

TEST_F(BusTest, ReadWrite8Failure) {
  // arrange
  uint8_t value = 0x56;
  uint16_t addr = 0x0800;

  // act
  bus->write8(addr, value);
  auto result = bus->read8(addr);

  // assert
  ASSERT_NE(result, value);
  ASSERT_EQ(result, 0x00);
}

TEST_F(BusTest, ReadWrite16Success) {
  // arrange
  uint16_t value = 0x5643;
  uint16_t addr = 0x0240;

  // act
  bus->write16(addr, value);
  auto result = bus->read16(addr);

  // assert
  ASSERT_EQ(result, value);
}

TEST_F(BusTest, ReadWrite16Failure) {
  // arrange
  uint16_t value = 0x5643;
  uint16_t addr = 0x0800;

  // act
  bus->write16(addr, value);
  auto result = bus->read16(addr);

  // assert
  ASSERT_NE(result, value);
  ASSERT_EQ(result, 0x0000);
}