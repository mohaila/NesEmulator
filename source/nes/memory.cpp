#include "memory.hpp"

Memory::Memory(uint16_t sa, uint16_t ea)
    : start(sa), end(ea), size(ea - sa + 1) {
  memory = (uint8_t*)calloc(size, sizeof(uint8_t));
}

Memory::~Memory() { free(memory); }

uint8_t Memory::read8(uint16_t addr) {
  auto offset = index(addr);
  if ((offset >= start) && (offset <= end)) {
    return memory[offset];
  }
  return 0x00;
}

void Memory::write8(uint16_t addr, uint8_t value) {
  auto offset = index(addr);
  if ((offset >= start) && (offset <= end)) {
    memory[offset] = value;
  }
}

bool Memory::validate8(uint16_t addr) {
  if ((addr >= start) && (addr <= end)) {
    return true;
  }
  for (auto& ma : mirrors) {
    uint16_t ea = ma + size - 1;
    if ((addr >= ma) && (addr <= ea)) {
      return true;
    }
  }
  return false;
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

void Memory::mirror(uint16_t addr, uint16_t count) {
  for (auto i = 0; i < count; i++) {
    uint16_t sa = addr + i * size;
    mirrors.push_back(sa);
  }
}

uint16_t Memory::index(uint16_t addr) {
  if ((addr >= start) && (addr <= end)) {
    return addr - start;
  }
  for (auto& ma : mirrors) {
    uint16_t ea = ma + size - 1;
    if ((addr >= ma) && (addr <= ea)) {
      return addr - ma;
    }
  }
  return 0xffff;
}