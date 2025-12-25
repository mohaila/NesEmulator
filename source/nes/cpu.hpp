#pragma once

#include "bus.hpp"

using std::string;
using std::unordered_map;

#define STACK_PAGE 0x0100
#define NMI_PROC_ADDR 0xfffa
#define RESET_PROC_ADDR 0xfffc
#define IRQ_PROC_ADDR 0xfffe

enum class Flags : uint8_t {
  C = 0x01,
  Z = 0x02,
  I = 0x04,
  D = 0x08,
  B = 0x10,
  U = 0x20,
  V = 0x40,
  N = 0x80,
};

enum class Addressing : uint8_t {
  Abs,   // Absolute
  Absx,  // Absolute indexed
  Absy,  // Absolute indexed
  Acc,   // Accumulator
  Imm,   // Immediate
  Imp,   // Implicit
  Ind,   // Indirect
  Indx,  // Indexed indirect
  Indy,  // Indirect indexed
  Rel,   // Relative
  Zp,    // Zero page
  Zpx,   // Zero page indexed
  Zpy,   // Zero page indexed
};

class CPU;

struct OpcodeInfo {
  string mnemonic;
  void (CPU::*resolve)() = nullptr;
  void (CPU::*execute)() = nullptr;
  uint8_t bytes;
  uint8_t cycles;
  bool penality;
};

class CPU {
  CPU(const CPU&) = delete;
  CPU& operator=(const CPU&) = delete;

 public:
  CPU(shared_ptr<Bus> abus) : bus(abus) { setOpcodesInfo(); }
  ~CPU() = default;

  void reset();
  void nmi();
  void irq();
  void clock(bool force = false);

 public:  // for testing
  // private:
  // flags operations
  void setFlag(Flags mask) { p |= static_cast<uint8_t>(mask); }
  void clearFlag(Flags mask) { p &= ~static_cast<uint8_t>(mask); }
  bool getFlag(Flags mask) const { return p & static_cast<uint8_t>(mask); }
  void setFlag(Flags mask, bool on) {
    if (on) {
      setFlag(mask);
    } else {
      clearFlag(mask);
    }
  }
  void setZN(uint8_t value) {
    setFlag(Flags::Z, value == 0);
    setFlag(Flags::N, value >= 0x80);
  }
  uint8_t read8();
  void write8(uint8_t value);
  uint16_t getAddress();

  // stack operations
  void push8(uint8_t value);
  void push16(uint16_t value);
  uint8_t pop8();
  uint16_t pop16();
  // addressing
  uint16_t read16bug(uint16_t addr);
  void abs();
  void absx();
  void absy();
  void acc();
  void imm();
  void imp();
  void ind();
  void indx();
  void indy();
  void rel();
  void zp();
  void zpx();
  void zpy();
  // instructions
  void setOpcodesInfo();
  void branch(bool condition);
  void ADC();
  void AND();
  void ASL();
  void BCC();
  void BCS();
  void BEQ();
  void BIT();
  void BMI();
  void BNE();
  void BPL();

  shared_ptr<Bus> bus;
  uint8_t a = 0;
  uint8_t x = 0;
  uint8_t y = 0;
  uint8_t p = 0x24;
  uint8_t sp = 0xff;
  uint16_t pc = 0x0000;
  Addressing addressing = Addressing::Imp;
  uint16_t address = 0x0000;
  bool penality = false;
  uint16_t cycles = 0;
  OpcodeInfo opcodeInfo;
  unordered_map<uint8_t, OpcodeInfo> opcodes{};
};