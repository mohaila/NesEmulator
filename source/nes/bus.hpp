#pragma once

#include "memory.hpp"

using std::shared_ptr;
using std::vector;

class Bus {
  Bus(const Bus&) = delete;
  Bus& operator=(const Bus&) = delete;

 public:
  Bus() = default;
  ~Bus() = default;
  void connect(shared_ptr<Memory> memory) { this->memory = memory; }

  uint8_t read8(uint16_t addr) { return memory->read8(addr); }

  void write8(uint16_t addr, uint8_t value) { memory->write8(addr, value); }

  uint16_t read16(uint16_t addr) { return memory->read16(addr); }

  void write16(uint16_t addr, uint16_t value) { memory->write16(addr, value); }

 private:
  shared_ptr<Memory> memory = nullptr;
};