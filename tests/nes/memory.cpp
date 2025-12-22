#include <gtest/gtest.h>

#include "nes/memory.hpp"

using std::make_shared;
using std::shared_ptr;
using testing::Test;

class MemoryTest : public Test {
 protected:
  shared_ptr<Memory> memory = nullptr;

 protected:
  void SetUp() override { memory = make_shared<Memory>(0x0000, 0x07ff); }

  void TearDown() override { memory.reset(); }
};

TEST_F(MemoryTest, ReadWrite8Success) {
  // arrange
  uint8_t value = 0x56;
  uint16_t addr = 0x0240;

  // act
  memory->write8(addr, value);
  auto result = memory->read8(addr);

  // assert
  ASSERT_EQ(memory->getSize(), 0x0800);
  ASSERT_EQ(result, value);
}

TEST_F(MemoryTest, ReadWrite8Failure) {
  // arrange
  uint8_t value = 0x56;
  uint16_t addr = 0x0800;

  // act
  memory->write8(addr, value);
  auto result = memory->read8(addr);

  // assert
  ASSERT_NE(result, value);
  ASSERT_EQ(result, 0x00);
}

TEST_F(MemoryTest, ReadWrite16Success) {
  // arrange
  uint16_t value = 0x5643;
  uint16_t addr = 0x0240;

  // act
  memory->write16(addr, value);
  auto result = memory->read16(addr);

  // assert
  ASSERT_EQ(result, value);
}

TEST_F(MemoryTest, ReadWrite16Failure) {
  // arrange
  uint16_t value = 0x5643;
  uint16_t addr = 0x0800;

  // act
  memory->write16(addr, value);
  auto result = memory->read16(addr);

  // assert
  ASSERT_NE(result, value);
  ASSERT_EQ(result, 0x0000);
}

TEST_F(MemoryTest, Validate8Success) {
  // arrange
  uint16_t addr = 0x0240;

  // act
  auto result = memory->validate8(addr);

  // assert
  ASSERT_EQ(result, true);
}

TEST_F(MemoryTest, Validate8Failure) {
  // arrange
  uint16_t addr = 0x0800;

  // act
  auto result = memory->validate8(addr);

  // assert
  ASSERT_EQ(result, false);
}

TEST_F(MemoryTest, Validate16Success) {
  // arrange
  uint16_t addr = 0x0240;

  // act
  auto result = memory->validate16(addr);

  // assert
  ASSERT_EQ(result, true);
}

TEST_F(MemoryTest, Validate16Failure) {
  // arrange
  uint16_t addr = 0x07ff;

  // act
  auto result = memory->validate16(addr);

  // assert
  ASSERT_EQ(result, false);
}

TEST_F(MemoryTest, Mirror1) {
  // arrange
  uint8_t value = 0x56;
  uint16_t addr = 0x0240;
  uint16_t maddr = 0x1240;

  // act
  memory->mirror(0x1000);
  memory->write8(addr, value);
  auto result = memory->validate8(addr);
  auto byte = memory->read8(maddr);
  auto mirrors = memory->getMirrors();

  // assert
  ASSERT_EQ(result, true);
  ASSERT_EQ(byte, value);
  ASSERT_EQ(mirrors.size(), 1);
  ASSERT_EQ(mirrors.at(0), 0x1000);
}

TEST_F(MemoryTest, Mirror3) {
  // arrange
  uint8_t value = 0x56;
  uint16_t addr = 0x0240;
  uint16_t maddr = 0x1240;

  // act
  memory->mirror(0x0800, 3);
  memory->write8(addr, value);
  auto result = memory->validate8(maddr);
  auto byte = memory->read8(maddr);
  auto mirrors = memory->getMirrors();

  // assert
  ASSERT_EQ(result, true);
  ASSERT_EQ(byte, value);
  ASSERT_EQ(mirrors.size(), 3);
  ASSERT_EQ(mirrors.at(0), 0x0800);
  ASSERT_EQ(mirrors.at(1), 0x1000);
  ASSERT_EQ(mirrors.at(2), 0x1800);
}

TEST_F(MemoryTest, SetVector) {
  // arrange
  uint16_t addr = 0x01ff;
  vector<uint8_t> data{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                       0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

  // act
  memory->set(addr, data);
  auto byte0 = memory->read8(addr++);
  auto byte1 = memory->read8(addr++);
  auto byte2 = memory->read8(addr++);
  auto byte3 = memory->read8(addr++);
  auto byte4 = memory->read8(addr++);
  auto byte5 = memory->read8(addr++);
  auto byte6 = memory->read8(addr++);
  auto byte7 = memory->read8(addr++);
  auto byte8 = memory->read8(addr++);
  auto byte9 = memory->read8(addr++);
  auto byte10 = memory->read8(addr++);
  auto byte11 = memory->read8(addr++);
  auto byte12 = memory->read8(addr++);
  auto byte13 = memory->read8(addr++);
  auto byte14 = memory->read8(addr++);

  // assert
  auto i = 0;
  ASSERT_EQ(byte0, data.at(i++));
  ASSERT_EQ(byte1, data.at(i++));
  ASSERT_EQ(byte2, data.at(i++));
  ASSERT_EQ(byte3, data.at(i++));
  ASSERT_EQ(byte4, data.at(i++));
  ASSERT_EQ(byte5, data.at(i++));
  ASSERT_EQ(byte6, data.at(i++));
  ASSERT_EQ(byte7, data.at(i++));
  ASSERT_EQ(byte8, data.at(i++));
  ASSERT_EQ(byte9, data.at(i++));
  ASSERT_EQ(byte10, data.at(i++));
  ASSERT_EQ(byte11, data.at(i++));
  ASSERT_EQ(byte12, data.at(i++));
  ASSERT_EQ(byte13, data.at(i++));
  ASSERT_EQ(byte14, data.at(i++));
}

TEST_F(MemoryTest, SetArray) {
  // arrange
  uint16_t addr = 0x01ff;
  uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

  // act
  memory->set(addr, data, 15);
  auto byte0 = memory->read8(addr++);
  auto byte1 = memory->read8(addr++);
  auto byte2 = memory->read8(addr++);
  auto byte3 = memory->read8(addr++);
  auto byte4 = memory->read8(addr++);
  auto byte5 = memory->read8(addr++);
  auto byte6 = memory->read8(addr++);
  auto byte7 = memory->read8(addr++);
  auto byte8 = memory->read8(addr++);
  auto byte9 = memory->read8(addr++);
  auto byte10 = memory->read8(addr++);
  auto byte11 = memory->read8(addr++);
  auto byte12 = memory->read8(addr++);
  auto byte13 = memory->read8(addr++);
  auto byte14 = memory->read8(addr++);

  // assert
  auto i = 0;
  ASSERT_EQ(byte0, data[i++]);
  ASSERT_EQ(byte1, data[i++]);
  ASSERT_EQ(byte2, data[i++]);
  ASSERT_EQ(byte3, data[i++]);
  ASSERT_EQ(byte4, data[i++]);
  ASSERT_EQ(byte5, data[i++]);
  ASSERT_EQ(byte6, data[i++]);
  ASSERT_EQ(byte7, data[i++]);
  ASSERT_EQ(byte8, data[i++]);
  ASSERT_EQ(byte9, data[i++]);
  ASSERT_EQ(byte10, data[i++]);
  ASSERT_EQ(byte11, data[i++]);
  ASSERT_EQ(byte12, data[i++]);
  ASSERT_EQ(byte13, data[i++]);
  ASSERT_EQ(byte14, data[i++]);
}