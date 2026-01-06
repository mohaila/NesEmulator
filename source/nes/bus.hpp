#pragma once

#include "device.hpp"

using std::shared_ptr;

class Bus {
  Bus(const Bus&) = delete;
  Bus& operator=(const Bus&) = delete;

 public:
  Bus() = default;
  virtual ~Bus() = default;

  virtual void connect(shared_ptr<Device> device) = 0;
  virtual uint8_t read8(uint16_t addr) = 0;
  virtual void write8(uint16_t addr, uint8_t value) = 0;
  virtual uint16_t read16(uint16_t addr) = 0;
  virtual void write16(uint16_t addr, uint16_t value) = 0;
};
