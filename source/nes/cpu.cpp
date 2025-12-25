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
  // CLC
  opcodes[0x18] = OpcodeInfo{"CLC", &CPU::imp, &CPU::CLC, 1, 2, false};
  // CLD
  opcodes[0xd8] = OpcodeInfo{"CLD", &CPU::imp, &CPU::CLD, 1, 2, false};
  // CLI
  opcodes[0x58] = OpcodeInfo{"CLI", &CPU::imp, &CPU::CLI, 1, 2, false};
  // CLV
  opcodes[0xb8] = OpcodeInfo{"CLV", &CPU::imp, &CPU::CLV, 1, 2, false};
  // CMP
  opcodes[0xc9] = OpcodeInfo{"CMP", &CPU::imm, &CPU::CMP, 2, 2, false};
  opcodes[0xc5] = OpcodeInfo{"CMP", &CPU::zp, &CPU::CMP, 2, 3, false};
  opcodes[0xd5] = OpcodeInfo{"CMP", &CPU::zpx, &CPU::CMP, 2, 4, false};
  opcodes[0xcd] = OpcodeInfo{"CMP", &CPU::abs, &CPU::CMP, 3, 4, false};
  opcodes[0xdd] = OpcodeInfo{"CMP", &CPU::absx, &CPU::CMP, 3, 4, true};
  opcodes[0xd9] = OpcodeInfo{"CMP", &CPU::absy, &CPU::CMP, 3, 4, true};
  opcodes[0xc1] = OpcodeInfo{"CMP", &CPU::indx, &CPU::CMP, 2, 6, false};
  opcodes[0xd1] = OpcodeInfo{"CMP", &CPU::indy, &CPU::CMP, 2, 5, true};
  // CPX
  opcodes[0xe0] = OpcodeInfo{"CPX", &CPU::imm, &CPU::CPX, 2, 2, false};
  opcodes[0xe4] = OpcodeInfo{"CPX", &CPU::zp, &CPU::CPX, 2, 3, false};
  opcodes[0xec] = OpcodeInfo{"CPX", &CPU::abs, &CPU::CPX, 3, 4, false};
  // CPX
  opcodes[0xc0] = OpcodeInfo{"CPY", &CPU::imm, &CPU::CPY, 2, 2, false};
  opcodes[0xc4] = OpcodeInfo{"CPY", &CPU::zp, &CPU::CPY, 2, 3, false};
  opcodes[0xcc] = OpcodeInfo{"CPY", &CPU::abs, &CPU::CPY, 3, 4, false};
  // DEC
  opcodes[0xc6] = OpcodeInfo{"DEC", &CPU::zp, &CPU::DEC, 2, 5, false};
  opcodes[0xd6] = OpcodeInfo{"DEC", &CPU::zpx, &CPU::DEC, 2, 6, false};
  opcodes[0xce] = OpcodeInfo{"DEC", &CPU::abs, &CPU::DEC, 3, 6, false};
  opcodes[0xde] = OpcodeInfo{"DEC", &CPU::absx, &CPU::DEC, 3, 7, true};
  // DEX
  opcodes[0xca] = OpcodeInfo{"DEX", &CPU::imp, &CPU::DEX, 1, 2, false};
  // DEY
  opcodes[0x88] = OpcodeInfo{"DEY", &CPU::imp, &CPU::DEY, 1, 2, false};
  // EOR
  opcodes[0x49] = OpcodeInfo{"EOR", &CPU::imm, &CPU::EOR, 2, 2, false};
  opcodes[0x45] = OpcodeInfo{"EOR", &CPU::zp, &CPU::EOR, 2, 3, false};
  opcodes[0x55] = OpcodeInfo{"EOR", &CPU::zpx, &CPU::EOR, 2, 4, false};
  opcodes[0x4d] = OpcodeInfo{"EOR", &CPU::abs, &CPU::EOR, 3, 4, false};
  opcodes[0x5d] = OpcodeInfo{"EOR", &CPU::absx, &CPU::EOR, 3, 4, true};
  opcodes[0x59] = OpcodeInfo{"EOR", &CPU::absy, &CPU::EOR, 3, 4, true};
  opcodes[0x41] = OpcodeInfo{"EOR", &CPU::indx, &CPU::EOR, 2, 6, false};
  opcodes[0x51] = OpcodeInfo{"EOR", &CPU::indy, &CPU::EOR, 2, 5, true};
  // INC
  opcodes[0xe6] = OpcodeInfo{"INC", &CPU::zp, &CPU::INC, 2, 5, false};
  opcodes[0xf6] = OpcodeInfo{"INC", &CPU::zpx, &CPU::INC, 2, 6, false};
  opcodes[0xee] = OpcodeInfo{"INC", &CPU::abs, &CPU::INC, 3, 6, false};
  opcodes[0xfe] = OpcodeInfo{"INC", &CPU::absx, &CPU::INC, 3, 7, true};
  // INX
  opcodes[0xe8] = OpcodeInfo{"INX", &CPU::imp, &CPU::INX, 1, 2, false};
  // INY
  opcodes[0xc8] = OpcodeInfo{"INY", &CPU::imp, &CPU::INY, 1, 2, false};
  // JMP
  opcodes[0x4c] = OpcodeInfo{"JMP", &CPU::abs, &CPU::JMP, 3, 3, false};
  opcodes[0x6c] = OpcodeInfo{"JMP", &CPU::ind, &CPU::JMP, 3, 5, false};
  // JSR
  opcodes[0x20] = OpcodeInfo{"JSR", &CPU::abs, &CPU::JSR, 3, 6, false};
  // LDA
  opcodes[0xa9] = OpcodeInfo{"LDA", &CPU::imm, &CPU::LDA, 2, 2, false};
  opcodes[0xa5] = OpcodeInfo{"LDA", &CPU::zp, &CPU::LDA, 2, 3, false};
  opcodes[0xb5] = OpcodeInfo{"LDA", &CPU::zpx, &CPU::LDA, 2, 4, false};
  opcodes[0xad] = OpcodeInfo{"LDA", &CPU::abs, &CPU::LDA, 3, 4, false};
  opcodes[0xbd] = OpcodeInfo{"LDA", &CPU::absx, &CPU::LDA, 3, 4, true};
  opcodes[0xb9] = OpcodeInfo{"LDA", &CPU::absy, &CPU::LDA, 3, 4, true};
  opcodes[0xa1] = OpcodeInfo{"LDA", &CPU::indx, &CPU::LDA, 2, 6, false};
  opcodes[0xb1] = OpcodeInfo{"LDA", &CPU::indy, &CPU::LDA, 2, 5, true};
  // LDX
  opcodes[0xa2] = OpcodeInfo{"LDX", &CPU::imm, &CPU::LDX, 2, 2, false};
  opcodes[0xa6] = OpcodeInfo{"LDX", &CPU::zp, &CPU::LDX, 2, 3, false};
  opcodes[0xb6] = OpcodeInfo{"LDX", &CPU::zpy, &CPU::LDX, 2, 4, false};
  opcodes[0xae] = OpcodeInfo{"LDX", &CPU::abs, &CPU::LDX, 3, 4, false};
  opcodes[0xbe] = OpcodeInfo{"LDX", &CPU::absy, &CPU::LDX, 3, 4, true};
  // LDY
  opcodes[0xa0] = OpcodeInfo{"LDY", &CPU::imm, &CPU::LDY, 2, 2, false};
  opcodes[0xa4] = OpcodeInfo{"LDY", &CPU::zp, &CPU::LDY, 2, 3, false};
  opcodes[0xb4] = OpcodeInfo{"LDY", &CPU::zpx, &CPU::LDY, 2, 4, false};
  opcodes[0xac] = OpcodeInfo{"LDY", &CPU::abs, &CPU::LDY, 3, 4, false};
  opcodes[0xbc] = OpcodeInfo{"LDY", &CPU::absx, &CPU::LDY, 3, 4, true};
  // LSR
  opcodes[0x4a] = OpcodeInfo{"LSR", &CPU::acc, &CPU::LSR, 1, 2, false};
  opcodes[0x46] = OpcodeInfo{"LSR", &CPU::zp, &CPU::LSR, 2, 5, false};
  opcodes[0x56] = OpcodeInfo{"LSR", &CPU::zpx, &CPU::LSR, 2, 6, false};
  opcodes[0x4e] = OpcodeInfo{"LSR", &CPU::abs, &CPU::LSR, 3, 6, false};
  opcodes[0x5e] = OpcodeInfo{"LSR", &CPU::absx, &CPU::LSR, 3, 7, false};
  // NOP
  opcodes[0xea] = OpcodeInfo{"NOP", &CPU::imp, &CPU::NOP, 1, 2, false};
  // ORA
  opcodes[0x09] = OpcodeInfo{"ORA", &CPU::imm, &CPU::ORA, 2, 2, false};
  opcodes[0x05] = OpcodeInfo{"ORA", &CPU::zp, &CPU::ORA, 2, 3, false};
  opcodes[0x15] = OpcodeInfo{"ORA", &CPU::zpx, &CPU::ORA, 2, 4, false};
  opcodes[0x0d] = OpcodeInfo{"ORA", &CPU::abs, &CPU::ORA, 3, 4, false};
  opcodes[0x1d] = OpcodeInfo{"ORA", &CPU::absx, &CPU::ORA, 3, 4, true};
  opcodes[0x19] = OpcodeInfo{"ORA", &CPU::absy, &CPU::ORA, 3, 4, true};
  opcodes[0x01] = OpcodeInfo{"ORA", &CPU::indx, &CPU::ORA, 2, 6, false};
  opcodes[0x11] = OpcodeInfo{"ORA", &CPU::indy, &CPU::ORA, 2, 5, true};
  // PHA
  opcodes[0x48] = OpcodeInfo{"PHA", &CPU::imp, &CPU::PHA, 1, 3, false};
  // PHP
  opcodes[0x08] = OpcodeInfo{"PHP", &CPU::imp, &CPU::PHP, 1, 3, false};
  // PLA
  opcodes[0x68] = OpcodeInfo{"PLA", &CPU::imp, &CPU::PLA, 1, 4, false};
  // PLP
  opcodes[0x28] = OpcodeInfo{"PLP", &CPU::imp, &CPU::PLP, 1, 4, false};
  // ROL
  opcodes[0x2a] = OpcodeInfo{"ROL", &CPU::acc, &CPU::ROL, 1, 2, false};
  opcodes[0x26] = OpcodeInfo{"ROL", &CPU::zp, &CPU::ROL, 2, 5, false};
  opcodes[0x36] = OpcodeInfo{"ROL", &CPU::zpx, &CPU::ROL, 2, 6, false};
  opcodes[0x2e] = OpcodeInfo{"ROL", &CPU::abs, &CPU::ROL, 3, 6, false};
  opcodes[0x3e] = OpcodeInfo{"ROL", &CPU::absx, &CPU::ROL, 3, 7, false};
  // ROR
  opcodes[0x6a] = OpcodeInfo{"ROR", &CPU::acc, &CPU::ROR, 1, 2, false};
  opcodes[0x66] = OpcodeInfo{"ROR", &CPU::zp, &CPU::ROR, 2, 5, false};
  opcodes[0x76] = OpcodeInfo{"ROR", &CPU::zpx, &CPU::ROR, 2, 6, false};
  opcodes[0x6e] = OpcodeInfo{"ROR", &CPU::abs, &CPU::ROR, 3, 6, false};
  opcodes[0x7e] = OpcodeInfo{"ROR", &CPU::absx, &CPU::ROR, 3, 7, false};
  // RTI
  opcodes[0x40] = OpcodeInfo{"RTI", &CPU::imp, &CPU::RTI, 1, 6, false};
  // RTS
  opcodes[0x60] = OpcodeInfo{"RTS", &CPU::imp, &CPU::RTS, 1, 6, false};
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

void CPU::compare(uint8_t r) {
  auto value = read8();
  setFlag(Flags::C, r >= value);
  setFlag(Flags::Z, r == value);
  setFlag(Flags::N, ((r - value) & 0x80) != 0);
  if (opcodeInfo.penality && penality) {
    cycles++;
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

void CPU::CLC() { clearFlag(Flags::C); }

void CPU::CLD() { clearFlag(Flags::D); }

void CPU::CLI() { clearFlag(Flags::I); }

void CPU::CLV() { clearFlag(Flags::V); }

void CPU::CMP() { compare(a); }

void CPU::CPX() { compare(x); }

void CPU::CPY() { compare(y); }

void CPU::DEC() {
  auto value = read8() - 1;
  setZN(value);
  write8(value);
}

void CPU::DEX() {
  x--;
  setZN(x);
}

void CPU::DEY() {
  y--;
  setZN(y);
}

void CPU::EOR() {
  a ^= read8();
  setZN(a);
  if (opcodeInfo.penality && penality) {
    cycles++;
  }
}

void CPU::INC() {
  auto value = read8() + 1;
  setZN(value);
  write8(value);
}

void CPU::INX() {
  x++;
  setZN(x);
}

void CPU::INY() {
  y++;
  setZN(y);
}

void CPU::JMP() { pc = address; }

void CPU::JSR() {
  push16(pc);
  pc = address;
}

void CPU::LDA() {
  a = read8();
  setZN(a);
}

void CPU::LDX() {
  x = read8();
  setZN(x);
}

void CPU::LDY() {
  y = read8();
  setZN(y);
}

void CPU::LSR() {
  auto value = read8();
  setFlag(Flags::C, (value & 0x01) != 0);
  value >>= 1;
  setZN(value);
  write8(value);
}

void CPU::NOP() {}

void CPU::ORA() {
  a |= read8();
  setZN(a);
}

void CPU::PHA() { push8(a); }

void CPU::PHP() {
  auto status =
      p | static_cast<uint8_t>(Flags::B) | static_cast<uint8_t>(Flags::U);
  push8(status);
}

void CPU::PLA() {
  a = pop8();
  setZN(a);
}

void CPU::PLP() { p = pop8() & 0xef | 0x20; }

void CPU::ROL() {
  auto value = read8();
  uint8_t c = getFlag(Flags::C) ? 0x01 : 0x00;
  setFlag(Flags::C, (value & 0x80) != 0);
  value = (value << 1) | c;
  setZN(value);
  write8(value);
}

void CPU::ROR() {
  auto value = read8();
  uint8_t c = getFlag(Flags::C) ? 0x01 : 0x00;
  setFlag(Flags::C, (value & 0x01) != 0);
  value = (c << 7) | (value >> 1);
  setZN(value);
  write8(value);
}

void CPU::RTI() {
  p = pop8() & 0xef | 0x20;
  pc = pop16();
}

void CPU::RTS() { pc = pop16(); }