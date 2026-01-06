#pragma once

#pragma once

#include "bus.hpp"
#include "memory.hpp"

class MemoryBus final : public Bus {
  MemoryBus(const MemoryBus&) = delete;
  MemoryBus& operator=(const MemoryBus&) = delete;

 public:
  MemoryBus() = default;
  ~MemoryBus() = default;

  virtual void connect(shared_ptr<Device> device) override;
  virtual uint8_t read8(uint16_t addr) override;
  virtual void write8(uint16_t addr, uint8_t value) override;
  virtual uint16_t read16(uint16_t addr) override;
  virtual void write16(uint16_t addr, uint16_t value) override;

 private:
  shared_ptr<Memory> memory = nullptr;
};