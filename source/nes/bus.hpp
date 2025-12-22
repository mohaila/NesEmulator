#pragma once

#include "device.hpp"

using std::shared_ptr;
using std::vector;

class Bus {
  Bus(const Bus&) = delete;
  Bus& operator=(const Bus&) = delete;

 public:
  Bus() = default;
  ~Bus() = default;
  void add(shared_ptr<Device>& device) { devices.push_back(device); }

  uint8_t read8(uint16_t addr) {
    for (auto& device : devices) {
      if (device->validate8(addr)) {
        return device->read8(addr);
      }
    }
    return 0x00;
  }

  void write8(uint16_t addr, uint8_t value) {
    for (auto& device : devices) {
      if (device->validate8(addr)) {
        device->write8(addr, value);
        return;
      }
    }
  }

  uint16_t read16(uint16_t addr) {
    for (auto& device : devices) {
      if (device->validate16(addr)) {
        return device->read16(addr);
      }
    }
    return 0x0000;
  }

  void write16(uint16_t addr, uint16_t value) {
    for (auto& device : devices) {
      if (device->validate16(addr)) {
        device->write16(addr, value);
        return;
      }
    }
  }

 private:
  vector<shared_ptr<Device>> devices{};
};