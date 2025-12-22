#pragma once

#include "pch.h"

class Device {
  Device(const Device&) = delete;
  Device& operator=(const Device&) = delete;

 public:
  Device() = default;
  virtual ~Device() = default;

  virtual uint8_t read8(uint16_t addr) = 0;
  virtual void write8(uint16_t addr, uint8_t value) = 0;
  virtual bool validate8(uint16_t addr) = 0;
  virtual void mirror(uint16_t addr, uint16_t count = 1) {}

  uint16_t read16(uint16_t addr);
  void write16(uint16_t addr, uint_fast16_t value);
  bool validate16(uint16_t addr);
};