#include "cpu.hpp"

using std::exception;

void CPU::push8(uint8_t value) {
  uint16_t addr = STACK_PAGE + sp;
  bus->write8(addr, value);
  sp--;
}

void CPU::push16(uint16_t value) {
  uint8_t hi = (value & 0xff00) >> 8;
  push8(hi);
  uint8_t lo = value & 0x00ff;
  push8(lo);
}

uint8_t CPU::pop8() {
  sp++;
  uint16_t addr = STACK_PAGE + sp;
  return bus->read8(addr);
}

uint16_t CPU::pop16() {
  uint16_t lo = pop8();
  uint16_t hi = pop8();
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
  address = bus->read16(pc + 1);
}

void CPU::absx() {
  addressing = Addressing::Absx;
  address = bus->read16(pc + 1);
  auto page = address & 0xff00;
  address += x;
  penality = page != (address & 0xff00);
}

void CPU::absy() {
  addressing = Addressing::Absy;
  address = bus->read16(pc + 1);
  auto page = address & 0xff00;
  address += y;
  penality = page != (address & 0xff00);
}

void CPU::acc() { addressing = Addressing::Acc; }

void CPU::imm() {
  addressing = Addressing::Imm;
  address = pc + 1;
}

void CPU::imp() { addressing = Addressing::Imp; }

void CPU::ind() {
  addressing = Addressing::Ind;
  auto ptr = bus->read16(pc + 1);
  address = read16bug(ptr);
}

void CPU::indx() {
  addressing = Addressing::Indx;
  uint16_t ptr = uint16_t(bus->read8(pc + 1)) + x;
  address = read16bug(ptr);
}

void CPU::indy() {
  addressing = Addressing::Indy;
  auto ptr = bus->read8(pc + 1);
  address = read16bug(ptr);
  auto page = address & 0xff00;
  address += y;
  penality = page != (address & 0xff00);
}

void CPU::rel() {
  addressing = Addressing::Rel;
  auto offset = bus->read8(pc + 1);
  if (offset < 0x80) {
    address = pc + offset;
  } else {
    address = pc + 0x100 - offset;
  }
}

void CPU::zp() {
  addressing = Addressing::Zp;
  address = bus->read8(pc + 1);
}

void CPU::zpx() {
  addressing = Addressing::Zpx;
  address = (bus->read8(pc + 1) + x) & 0x00ff;
}

void CPU::zpy() {
  addressing = Addressing::Zpy;
  address = (bus->read8(pc + 1) + y) & 0x00ff;
}

uint8_t CPU::read8() {
  switch (addressing) {
    case Addressing::Acc:
      return a;
    case Addressing::Imm:
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
  cycles += 7;
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
  cycles += 7;
}

void CPU::clock(bool force) {
  if (cycles == 0 || force) {
    try {
      auto opcode = bus->read8(pc);
      opcodeInfo = opcodes.at(opcode);
      (this->*opcodeInfo.resolve)();
      cycles += opcodeInfo.cycles;
      pc += opcodeInfo.bytes;
      (this->*opcodeInfo.execute)();
    } catch (const exception& e) {
    }
  }
  cycles--;
}

void CPU::setOpcodesInfo() {
  // ADC
  opcodes[0x69] = OpcodeInfo{"ADC", &CPU::imm, &CPU::ADC, 2, 2, false};
  opcodes[0x65] = OpcodeInfo{"ADC", &CPU::zp, &CPU::ADC, 2, 3, false};
  opcodes[0x75] = OpcodeInfo{"ADC", &CPU::zpx, &CPU::ADC, 2, 4, false};
  opcodes[0x6d] = OpcodeInfo{"ADC", &CPU::abs, &CPU::ADC, 3, 4, false};
  opcodes[0x7d] = OpcodeInfo{"ADC", &CPU::absx, &CPU::ADC, 3, 4, true};
  opcodes[0x79] = OpcodeInfo{"ADC", &CPU::absy, &CPU::ADC, 3, 4, true};
  opcodes[0x61] = OpcodeInfo{"ADC", &CPU::indx, &CPU::ADC, 2, 6, false};
  opcodes[0x71] = OpcodeInfo{"ADC", &CPU::indy, &CPU::ADC, 2, 5, true};
  // AND
  opcodes[0x29] = OpcodeInfo{"AND", &CPU::imm, &CPU::AND, 2, 2, false};
  opcodes[0x25] = OpcodeInfo{"AND", &CPU::zp, &CPU::AND, 2, 3, false};
  opcodes[0x35] = OpcodeInfo{"AND", &CPU::zpx, &CPU::AND, 2, 4, false};
  opcodes[0x2d] = OpcodeInfo{"AND", &CPU::abs, &CPU::AND, 3, 4, false};
  opcodes[0x3d] = OpcodeInfo{"AND", &CPU::absx, &CPU::AND, 3, 4, true};
  opcodes[0x39] = OpcodeInfo{"AND", &CPU::absy, &CPU::AND, 3, 4, true};
  opcodes[0x21] = OpcodeInfo{"AND", &CPU::indx, &CPU::AND, 2, 6, false};
  opcodes[0x31] = OpcodeInfo{"AND", &CPU::indy, &CPU::AND, 2, 5, true};
  // ASL
  opcodes[0x0a] = OpcodeInfo{"ASL", &CPU::acc, &CPU::ASL, 1, 2, false};
  opcodes[0x06] = OpcodeInfo{"ASL", &CPU::zp, &CPU::ASL, 2, 5, false};
  opcodes[0x16] = OpcodeInfo{"ASL", &CPU::zpx, &CPU::ASL, 2, 6, false};
  opcodes[0x0e] = OpcodeInfo{"ASL", &CPU::abs, &CPU::ASL, 3, 6, false};
  opcodes[0x1e] = OpcodeInfo{"ASL", &CPU::absx, &CPU::ASL, 3, 7, false};
  // BCC
  opcodes[0x90] = OpcodeInfo{"BCC", &CPU::rel, &CPU::BCC, 2, 2, true};
  // BCS
  opcodes[0xb0] = OpcodeInfo{"BCS", &CPU::rel, &CPU::BCS, 2, 2, true};
  // BEQ
  opcodes[0xf0] = OpcodeInfo{"BEQ", &CPU::rel, &CPU::BEQ, 2, 2, true};
  // BIT
  opcodes[0x24] = OpcodeInfo{"BIT", &CPU::zp, &CPU::BIT, 2, 3, false};
  opcodes[0x2c] = OpcodeInfo{"BIT", &CPU::abs, &CPU::BIT, 3, 4, false};
  // BMI
  opcodes[0x30] = OpcodeInfo{"BMI", &CPU::rel, &CPU::BMI, 2, 2, true};
  // BNE
  opcodes[0xd0] = OpcodeInfo{"BNE", &CPU::rel, &CPU::BNE, 2, 2, true};
  // BPL
  opcodes[0x10] = OpcodeInfo{"BPL", &CPU::rel, &CPU::BPL, 2, 2, true};
  // BRK
  opcodes[0x00] = OpcodeInfo{"BRK", &CPU::imm, &CPU::BRK, 2, 7, false};
  // BVC
  opcodes[0x50] = OpcodeInfo{"BVC", &CPU::rel, &CPU::BVC, 2, 2, true};
  // BVS
  opcodes[0x70] = OpcodeInfo{"BVS", &CPU::rel, &CPU::BVS, 2, 2, true};
}

void CPU::branch(bool condition) {
  if (condition) {
    auto page = pc & 0xff00;
    pc = address + 2;
    cycles++;
    if (page != (pc & 0xff00)) {
      cycles++;
    }
  }
}

void CPU::ADC() {
  uint16_t c = getFlag(Flags::C) ? 1 : 0;
  uint16_t value = read8();
  uint16_t sum = uint16_t(a) + value + c;
  setFlag(Flags::C, sum > 0x00ff);
  uint8_t result = sum & 0x00ff;
  setFlag(Flags::V, ((result ^ a) & (result ^ value) & 0x80) != 0);
  a = result;
  setZN(a);
  if (opcodeInfo.penality && penality) {
    cycles += 1;
  }
}

void CPU::AND() {
  uint16_t value = read8();
  a &= value;
  setZN(a);
  if (opcodeInfo.penality && penality) {
    cycles += 1;
  }
}

void CPU::ASL() {
  auto value = read8();
  setFlag(Flags::C, (value & 0x80) != 0);
  value <<= 1;
  setZN(value);
  write8(value);
}

void CPU::BCC() { branch(!getFlag(Flags::C)); }

void CPU::BCS() { branch(getFlag(Flags::C)); }

void CPU::BEQ() { branch(getFlag(Flags::Z)); }

void CPU::BIT() {
  auto result = a & read8();
  setFlag(Flags::Z, a == 0);
  setFlag(Flags::N, (a & 0x80) != 0);
  setFlag(Flags::V, (a & 0x40) != 0);
}

void CPU::BMI() { branch(getFlag(Flags::N)); }

void CPU::BNE() { branch(!getFlag(Flags::Z)); }

void CPU::BPL() { branch(!getFlag(Flags::N)); }

void CPU::BRK() {
  push16(pc);
  pc = bus->read16(IRQ_PROC_ADDR);
  auto status =
      p | static_cast<uint8_t>(Flags::B) | static_cast<uint8_t>(Flags::U);
  push8(status);
  setFlag(Flags::I);
}

void CPU::BVC() { branch(!getFlag(Flags::V)); }

void CPU::BVS() { branch(getFlag(Flags::V)); }