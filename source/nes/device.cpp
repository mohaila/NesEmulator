#include "device.hpp"

uint16_t Device::read16(uint16_t addr) {
  auto lo = uint16_t(read8(addr));
  auto hi = uint16_t(read8(addr + 1));
  return (hi << 8) | lo;
}

void Device::write16(uint16_t addr, uint_fast16_t value) {
  uint8_t lo = value & 0x00ff;
  write8(addr, lo);
  uint8_t hi = (value & 0xff00) >> 8;
  write8(addr + 1, hi);
}
