#pragma once

#include "device.hpp"

using std::vector;

class Memory : public Device {
  Memory(const Memory&) = delete;
  Memory& operator=(const Memory&) = delete;

 public:
  Memory(uint16_t sa, uint16_t ea);
  virtual ~Memory();
  virtual uint8_t read8(uint16_t addr) override;
  virtual void write8(uint16_t addr, uint8_t value) override;
  virtual bool validate8(uint16_t addr) override;
  virtual void mirror(uint16_t addr, uint16_t count = 1) override;

  void set(uint16_t addr, const vector<uint8_t>& data);
  void set(uint16_t addr, const uint8_t* data, const uint32_t len);

  uint32_t getSize() const { return size; }
  const vector<uint16_t>& getMirrors() const { return mirrors; }

 private:
  uint16_t index(uint16_t addr);

 private:
  uint16_t start;
  uint16_t end;
  uint32_t size;
  uint8_t* memory;
  vector<uint16_t> mirrors{};
};