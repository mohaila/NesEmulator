#include "cpu.hpp"

void CPU::push8(uint8_t value) {
  uint16_t addr = STACK_PAGE + sp;
  bus->write8(addr, value);
  sp--;
}

void CPU::push16(uint16_t value) {
  uint8_t lo = value & 0x00ff;
  push8(lo);
  uint8_t hi = (value & 0xff00) >> 8;
  push8(hi);
}

uint8_t CPU::pop8() {
  sp++;
  uint16_t addr = STACK_PAGE + sp;
  return bus->read8(addr);
}

uint16_t CPU::pop16() {
  uint16_t hi = pop8();
  uint16_t lo = pop8();
  return (hi << 8) | lo;
}

uint16_t CPU::read16bug(uint16_t addr) {
  uint16_t lo = bus->read8(addr);
  auto baddr = (addr & 0xff00) | ((addr + 1) & 0x00ff);
  uint16_t hi = bus->read8(baddr);
  return (hi << 8) | lo;
}

void CPU::abs() {
  addressing = Addressing::Abs;
  address = bus->read16(pc);
}

void CPU::absx() {
  addressing = Addressing::Absx;
  address = bus->read16(pc);
  auto page = address & 0xff00;
  address += x;
  penality = page != (address & 0xff00);
}

void CPU::absy() {
  addressing = Addressing::Absy;
  address = bus->read16(pc);
  auto page = address & 0xff00;
  address += y;
  penality = page != (address & 0xff00);
}

void CPU::acc() { addressing = Addressing::Acc; }

void CPU::imm() { addressing = Addressing::Imm; }

void CPU::imp() { addressing = Addressing::Imp; }

void CPU::ind() {
  addressing = Addressing::Ind;
  auto ptr = bus->read16(pc);
  address = read16bug(ptr);
}

void CPU::indx() {
  addressing = Addressing::Indx;
  uint16_t ptr = uint16_t(bus->read8(pc)) + x;
  address = read16bug(ptr);
}

void CPU::indy() {
  addressing = Addressing::Indy;
  auto ptr = bus->read16(pc);
  address = read16bug(ptr);
  auto page = address & 0xff00;
  address += y;
  penality = page != (address & 0xff00);
}

void CPU::rel() {
  addressing = Addressing::Rel;
  auto offset = bus->read8(pc);
  if (offset < 0x80) {
    address = pc + offset;
  } else {
    address = pc + 0x100 - offset;
  }
}

void CPU::zp() {
  addressing = Addressing::Zp;
  address = bus->read8(pc);
}

void CPU::zpx() {
  addressing = Addressing::Zpx;
  address = (bus->read8(pc) + x) & 0x00ff;
}

void CPU::zpy() {
  addressing = Addressing::Zpy;
  address = (bus->read8(pc) + y) & 0x00ff;
}

uint8_t CPU::read8() {
  switch (addressing) {
    case Addressing::Acc:
      return a;
    case Addressing::Imm:
      return bus->read8(pc);
    case Addressing::Abs:
    case Addressing::Absx:
    case Addressing::Absy:
    case Addressing::Ind:
    case Addressing::Indx:
    case Addressing::Indy:
    case Addressing::Zp:
    case Addressing::Zpx:
    case Addressing::Zpy:
      return bus->read8(address);
  }
  return 0x00;
}

void CPU::write8(uint8_t value) {
  switch (addressing) {
    case Addressing::Acc:
      a = value;
      break;
    case Addressing::Abs:
    case Addressing::Absx:
    case Addressing::Absy:
    case Addressing::Ind:
    case Addressing::Indx:
    case Addressing::Indy:
    case Addressing::Zp:
    case Addressing::Zpx:
    case Addressing::Zpy:
      bus->write8(address, value);
      break;
  }
}

uint16_t CPU::getAddress() {
  switch (addressing) {
    case Addressing::Abs:
    case Addressing::Absx:
    case Addressing::Absy:
    case Addressing::Ind:
    case Addressing::Indx:
    case Addressing::Indy:
    case Addressing::Zp:
    case Addressing::Zpx:
    case Addressing::Zpy:
    case Addressing::Rel:
      return address;
    case Addressing::Acc:
    case Addressing::Imp:
    case Addressing::Imm:
      break;
  }
  return 0x0000;
}

void CPU::reset() {
  a = 0;
  x = 0;
  y = 0;
  p = 0x24;
  sp = 0xff;
  pc = bus->read16(RESET_PROC_ADDR);
}

void CPU::nmi() {
  push16(pc);
  pc = bus->read16(NMI_PROC_ADDR);
  auto status =
      p & ~static_cast<uint8_t>(Flags::B) | static_cast<uint8_t>(Flags::U);
  push8(status);
  setFlag(Flags::I);
}

void CPU::irq() {
  if (getFlag(Flags::I)) {
    return;
  }
  push16(pc);
  pc = bus->read16(IRQ_PROC_ADDR);
  auto status =
      p & ~static_cast<uint8_t>(Flags::B) | static_cast<uint8_t>(Flags::U);
  push8(status);
  setFlag(Flags::I);
}