#include "memory.hpp"

Memory::Memory(uint16_t sa, uint16_t ea)
    : start(sa), end(ea), size(ea - sa + 1) {
  memory = (uint8_t*)calloc(size, sizeof(uint8_t));
}

Memory::~Memory() { free(memory); }

uint8_t Memory::read8(uint16_t addr) {
  auto offset = index(addr);
  return memory[offset];
}

void Memory::write8(uint16_t addr, uint8_t value) {
  auto offset = index(addr);
  memory[offset] = value;
}

void Memory::set(uint16_t addr, const vector<uint8_t>& data) {
  auto offset = index(addr);
  if (offset < size) {
    auto available = size - offset;
    auto len = data.size();
    auto copy = len > available ? available : len;
    memcpy(memory + offset, data.data(), copy);
  }
}

void Memory::set(uint16_t addr, const uint8_t* data, const uint32_t len) {
  auto offset = index(addr);
  if (offset < size) {
    auto available = size - offset;
    auto copy = len > available ? available : len;
    memcpy(memory + offset, data, copy);
  }
}
