#include "memorybus.hpp"

using std::reinterpret_pointer_cast;

void MemoryBus::connect(shared_ptr<Device> device) {
  this->memory = reinterpret_pointer_cast<Memory>(device);
}

uint8_t MemoryBus::read8(uint16_t addr) { return memory->read8(addr); }

void MemoryBus::write8(uint16_t addr, uint8_t value) {
  memory->write8(addr, value);
}

uint16_t MemoryBus::read16(uint16_t addr) { return memory->read16(addr); }

void MemoryBus::write16(uint16_t addr, uint16_t value) {
  memory->write16(addr, value);
}