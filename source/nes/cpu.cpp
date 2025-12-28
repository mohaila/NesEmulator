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
    address = pc + 2 + offset;
  } else {
    address = pc + 2 + offset - 0x100;
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
  p = 0x20;
  sp = 0xfd;
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
    auto opcode = bus->read8(pc);
    opcodeInfo = opcodes[opcode];
    if (opcodeInfo.mnemonic == "XXX") {
      fprintf(stderr, "Invalid opcode at %04x\n", pc);
      debug();
    }
    (this->*opcodeInfo.resolve)();
    cycles += opcodeInfo.cycles;
    pc += opcodeInfo.bytes;
    (this->*opcodeInfo.execute)();
  }
  cycles--;
}

void CPU::debug() {
  uint8_t byte1 = bus->read8(pc);
  char byte2[3] = "  ";
  if (opcodeInfo.bytes >= 2) {
    auto value = bus->read8(pc + 1);
    sprintf(byte2, "%02x", value);
  }
  char byte3[3] = "  ";
  if (opcodeInfo.bytes >= 3) {
    auto value = bus->read8(pc + 2);
    sprintf(byte3, "%02x", value);
  }

  printf("%04x %s  %02x %s %s a:%02x x:%02x y:%02x sp:%02x p:%08b\n", pc,
         opcodeInfo.mnemonic.c_str(), byte1, byte2, byte3, a, x, y, sp, p);
}

void CPU::setOpcodesInfo() {
  opcodes = {
      OpcodeInfo{0x0, "BRK", &CPU::imp, &CPU::BRK, 2, 7, false},
      OpcodeInfo{0x1, "ORA", &CPU::indx, &CPU::ORA, 2, 6, false},
      OpcodeInfo{0x2, "XXX", &CPU::imp, &CPU::XXX, 0, 2, false},
      OpcodeInfo{0x3, "XXX", &CPU::indx, &CPU::XXX, 0, 8, false},
      OpcodeInfo{0x4, "NOP", &CPU::zp, &CPU::NOP, 2, 3, false},
      OpcodeInfo{0x5, "ORA", &CPU::zp, &CPU::ORA, 2, 3, false},
      OpcodeInfo{0x6, "ASL", &CPU::zp, &CPU::ASL, 2, 5, false},
      OpcodeInfo{0x7, "XXX", &CPU::zp, &CPU::XXX, 0, 5, false},
      OpcodeInfo{0x8, "PHP", &CPU::imp, &CPU::PHP, 1, 3, false},
      OpcodeInfo{0x9, "ORA", &CPU::imm, &CPU::ORA, 2, 2, false},
      OpcodeInfo{0xa, "ASL", &CPU::acc, &CPU::ASL, 1, 2, false},
      OpcodeInfo{0xb, "XXX", &CPU::imm, &CPU::XXX, 0, 2, false},
      OpcodeInfo{0xc, "NOP", &CPU::abs, &CPU::NOP, 3, 4, false},
      OpcodeInfo{0xd, "ORA", &CPU::abs, &CPU::ORA, 3, 4, false},
      OpcodeInfo{0xe, "ASL", &CPU::abs, &CPU::ASL, 3, 6, false},
      OpcodeInfo{0xf, "XXX", &CPU::abs, &CPU::XXX, 0, 6, false},
      OpcodeInfo{0x10, "BPL", &CPU::rel, &CPU::BPL, 2, 2, true},
      OpcodeInfo{0x11, "ORA", &CPU::indy, &CPU::ORA, 2, 5, true},
      OpcodeInfo{0x12, "XXX", &CPU::imp, &CPU::XXX, 0, 2, false},
      OpcodeInfo{0x13, "XXX", &CPU::indy, &CPU::XXX, 0, 8, false},
      OpcodeInfo{0x14, "NOP", &CPU::zpx, &CPU::NOP, 2, 4, false},
      OpcodeInfo{0x15, "ORA", &CPU::zpx, &CPU::ORA, 2, 4, false},
      OpcodeInfo{0x16, "ASL", &CPU::zpx, &CPU::ASL, 2, 6, false},
      OpcodeInfo{0x17, "XXX", &CPU::zpx, &CPU::XXX, 0, 6, false},
      OpcodeInfo{0x18, "CLC", &CPU::imp, &CPU::CLC, 1, 2, false},
      OpcodeInfo{0x19, "ORA", &CPU::absy, &CPU::ORA, 3, 4, true},
      OpcodeInfo{0x1a, "NOP", &CPU::imp, &CPU::NOP, 1, 2, false},
      OpcodeInfo{0x1b, "XXX", &CPU::absy, &CPU::XXX, 0, 7, false},
      OpcodeInfo{0x1c, "NOP", &CPU::absx, &CPU::NOP, 3, 4, true},
      OpcodeInfo{0x1d, "ORA", &CPU::absx, &CPU::ORA, 3, 4, true},
      OpcodeInfo{0x1e, "ASL", &CPU::absx, &CPU::ASL, 3, 7, false},
      OpcodeInfo{0x1f, "XXX", &CPU::absx, &CPU::XXX, 0, 7, false},
      OpcodeInfo{0x20, "JSR", &CPU::abs, &CPU::JSR, 3, 6, false},
      OpcodeInfo{0x21, "AND", &CPU::indx, &CPU::AND, 2, 6, false},
      OpcodeInfo{0x22, "XXX", &CPU::imp, &CPU::XXX, 0, 2, false},
      OpcodeInfo{0x23, "XXX", &CPU::indx, &CPU::XXX, 0, 8, false},
      OpcodeInfo{0x24, "BIT", &CPU::zp, &CPU::BIT, 2, 3, false},
      OpcodeInfo{0x25, "AND", &CPU::zp, &CPU::AND, 2, 3, false},
      OpcodeInfo{0x26, "ROL", &CPU::zp, &CPU::ROL, 2, 5, false},
      OpcodeInfo{0x27, "XXX", &CPU::zp, &CPU::XXX, 0, 5, false},
      OpcodeInfo{0x28, "PLP", &CPU::imp, &CPU::PLP, 1, 4, false},
      OpcodeInfo{0x29, "AND", &CPU::imm, &CPU::AND, 2, 2, false},
      OpcodeInfo{0x2a, "ROL", &CPU::acc, &CPU::ROL, 1, 2, false},
      OpcodeInfo{0x2b, "XXX", &CPU::imm, &CPU::XXX, 0, 2, false},
      OpcodeInfo{0x2c, "BIT", &CPU::abs, &CPU::BIT, 3, 4, false},
      OpcodeInfo{0x2d, "AND", &CPU::abs, &CPU::AND, 3, 4, false},
      OpcodeInfo{0x2e, "ROL", &CPU::abs, &CPU::ROL, 3, 6, false},
      OpcodeInfo{0x2f, "XXX", &CPU::abs, &CPU::XXX, 0, 6, false},
      OpcodeInfo{0x30, "BMI", &CPU::rel, &CPU::BMI, 2, 2, true},
      OpcodeInfo{0x31, "AND", &CPU::indy, &CPU::AND, 2, 5, true},
      OpcodeInfo{0x32, "XXX", &CPU::imp, &CPU::XXX, 0, 2, false},
      OpcodeInfo{0x33, "XXX", &CPU::indy, &CPU::XXX, 0, 8, false},
      OpcodeInfo{0x34, "NOP", &CPU::zpx, &CPU::NOP, 2, 4, false},
      OpcodeInfo{0x35, "AND", &CPU::zpx, &CPU::AND, 2, 4, false},
      OpcodeInfo{0x36, "ROL", &CPU::zpx, &CPU::ROL, 2, 6, false},
      OpcodeInfo{0x37, "XXX", &CPU::zpx, &CPU::XXX, 0, 6, false},
      OpcodeInfo{0x38, "SEC", &CPU::imp, &CPU::SEC, 1, 2, false},
      OpcodeInfo{0x39, "AND", &CPU::absy, &CPU::AND, 3, 4, true},
      OpcodeInfo{0x3a, "NOP", &CPU::imp, &CPU::NOP, 1, 2, false},
      OpcodeInfo{0x3b, "XXX", &CPU::absy, &CPU::XXX, 0, 7, false},
      OpcodeInfo{0x3c, "NOP", &CPU::absx, &CPU::NOP, 3, 4, true},
      OpcodeInfo{0x3d, "AND", &CPU::absx, &CPU::AND, 3, 4, true},
      OpcodeInfo{0x3e, "ROL", &CPU::absx, &CPU::ROL, 3, 7, false},
      OpcodeInfo{0x3f, "XXX", &CPU::absx, &CPU::XXX, 0, 7, false},
      OpcodeInfo{0x40, "RTI", &CPU::imp, &CPU::RTI, 1, 6, false},
      OpcodeInfo{0x41, "EOR", &CPU::indx, &CPU::EOR, 2, 6, false},
      OpcodeInfo{0x42, "XXX", &CPU::imp, &CPU::XXX, 0, 2, false},
      OpcodeInfo{0x43, "XXX", &CPU::indx, &CPU::XXX, 0, 8, false},
      OpcodeInfo{0x44, "NOP", &CPU::zp, &CPU::NOP, 2, 3, false},
      OpcodeInfo{0x45, "EOR", &CPU::zp, &CPU::EOR, 2, 3, false},
      OpcodeInfo{0x46, "LSR", &CPU::zp, &CPU::LSR, 2, 5, false},
      OpcodeInfo{0x47, "XXX", &CPU::zp, &CPU::XXX, 0, 5, false},
      OpcodeInfo{0x48, "PHA", &CPU::imp, &CPU::PHA, 1, 3, false},
      OpcodeInfo{0x49, "EOR", &CPU::imm, &CPU::EOR, 2, 2, false},
      OpcodeInfo{0x4a, "LSR", &CPU::acc, &CPU::LSR, 1, 2, false},
      OpcodeInfo{0x4b, "XXX", &CPU::imm, &CPU::XXX, 0, 2, false},
      OpcodeInfo{0x4c, "JMP", &CPU::abs, &CPU::JMP, 3, 3, false},
      OpcodeInfo{0x4d, "EOR", &CPU::abs, &CPU::EOR, 3, 4, false},
      OpcodeInfo{0x4e, "LSR", &CPU::abs, &CPU::LSR, 3, 6, false},
      OpcodeInfo{0x4f, "XXX", &CPU::abs, &CPU::XXX, 0, 6, false},
      OpcodeInfo{0x50, "BVC", &CPU::rel, &CPU::BVC, 2, 2, true},
      OpcodeInfo{0x51, "EOR", &CPU::indy, &CPU::EOR, 2, 5, true},
      OpcodeInfo{0x52, "XXX", &CPU::imp, &CPU::XXX, 0, 2, false},
      OpcodeInfo{0x53, "XXX", &CPU::indy, &CPU::XXX, 0, 8, false},
      OpcodeInfo{0x54, "NOP", &CPU::zpx, &CPU::NOP, 2, 4, false},
      OpcodeInfo{0x55, "EOR", &CPU::zpx, &CPU::EOR, 2, 4, false},
      OpcodeInfo{0x56, "LSR", &CPU::zpx, &CPU::LSR, 2, 6, false},
      OpcodeInfo{0x57, "XXX", &CPU::zpx, &CPU::XXX, 0, 6, false},
      OpcodeInfo{0x58, "CLI", &CPU::imp, &CPU::CLI, 1, 2, false},
      OpcodeInfo{0x59, "EOR", &CPU::absy, &CPU::EOR, 3, 4, true},
      OpcodeInfo{0x5a, "NOP", &CPU::imp, &CPU::NOP, 1, 2, false},
      OpcodeInfo{0x5b, "XXX", &CPU::absy, &CPU::XXX, 0, 7, false},
      OpcodeInfo{0x5c, "NOP", &CPU::absx, &CPU::NOP, 3, 4, true},
      OpcodeInfo{0x5d, "EOR", &CPU::absx, &CPU::EOR, 3, 4, true},
      OpcodeInfo{0x5e, "LSR", &CPU::absx, &CPU::LSR, 3, 7, false},
      OpcodeInfo{0x5f, "XXX", &CPU::absx, &CPU::XXX, 0, 7, false},
      OpcodeInfo{0x60, "RTS", &CPU::imp, &CPU::RTS, 1, 6, false},
      OpcodeInfo{0x61, "ADC", &CPU::indx, &CPU::ADC, 2, 6, false},
      OpcodeInfo{0x62, "XXX", &CPU::imp, &CPU::XXX, 0, 2, false},
      OpcodeInfo{0x63, "XXX", &CPU::indx, &CPU::XXX, 0, 8, false},
      OpcodeInfo{0x64, "NOP", &CPU::zp, &CPU::NOP, 2, 3, false},
      OpcodeInfo{0x65, "ADC", &CPU::zp, &CPU::ADC, 2, 3, false},
      OpcodeInfo{0x66, "ROR", &CPU::zp, &CPU::ROR, 2, 5, false},
      OpcodeInfo{0x67, "XXX", &CPU::zp, &CPU::XXX, 0, 5, false},
      OpcodeInfo{0x68, "PLA", &CPU::imp, &CPU::PLA, 1, 4, false},
      OpcodeInfo{0x69, "ADC", &CPU::imm, &CPU::ADC, 2, 2, false},
      OpcodeInfo{0x6a, "ROR", &CPU::acc, &CPU::ROR, 1, 2, false},
      OpcodeInfo{0x6b, "XXX", &CPU::imm, &CPU::XXX, 0, 2, false},
      OpcodeInfo{0x6c, "JMP", &CPU::ind, &CPU::JMP, 3, 5, false},
      OpcodeInfo{0x6d, "ADC", &CPU::abs, &CPU::ADC, 3, 4, false},
      OpcodeInfo{0x6e, "ROR", &CPU::abs, &CPU::ROR, 3, 6, false},
      OpcodeInfo{0x6f, "XXX", &CPU::abs, &CPU::XXX, 0, 6, false},
      OpcodeInfo{0x70, "BVS", &CPU::rel, &CPU::BVS, 2, 2, true},
      OpcodeInfo{0x71, "ADC", &CPU::indy, &CPU::ADC, 2, 5, true},
      OpcodeInfo{0x72, "XXX", &CPU::imp, &CPU::XXX, 0, 2, false},
      OpcodeInfo{0x73, "XXX", &CPU::indy, &CPU::XXX, 0, 8, false},
      OpcodeInfo{0x74, "NOP", &CPU::zpx, &CPU::NOP, 2, 4, false},
      OpcodeInfo{0x75, "ADC", &CPU::zpx, &CPU::ADC, 2, 4, false},
      OpcodeInfo{0x76, "ROR", &CPU::zpx, &CPU::ROR, 2, 6, false},
      OpcodeInfo{0x77, "XXX", &CPU::zpx, &CPU::XXX, 0, 6, false},
      OpcodeInfo{0x78, "SEI", &CPU::imp, &CPU::SEI, 1, 2, false},
      OpcodeInfo{0x79, "ADC", &CPU::absy, &CPU::ADC, 3, 4, true},
      OpcodeInfo{0x7a, "NOP", &CPU::imp, &CPU::NOP, 1, 2, false},
      OpcodeInfo{0x7b, "XXX", &CPU::absy, &CPU::XXX, 0, 7, false},
      OpcodeInfo{0x7c, "NOP", &CPU::absx, &CPU::NOP, 3, 4, true},
      OpcodeInfo{0x7d, "ADC", &CPU::absx, &CPU::ADC, 3, 4, true},
      OpcodeInfo{0x7e, "ROR", &CPU::absx, &CPU::ROR, 3, 7, false},
      OpcodeInfo{0x7f, "XXX", &CPU::absx, &CPU::XXX, 0, 7, false},
      OpcodeInfo{0x80, "NOP", &CPU::imm, &CPU::NOP, 2, 2, false},
      OpcodeInfo{0x81, "STA", &CPU::indx, &CPU::STA, 2, 6, false},
      OpcodeInfo{0x82, "NOP", &CPU::imm, &CPU::NOP, 0, 2, false},
      OpcodeInfo{0x83, "XXX", &CPU::indx, &CPU::XXX, 0, 6, false},
      OpcodeInfo{0x84, "STY", &CPU::zp, &CPU::STY, 2, 3, false},
      OpcodeInfo{0x85, "STA", &CPU::zp, &CPU::STA, 2, 3, false},
      OpcodeInfo{0x86, "STX", &CPU::zp, &CPU::STX, 2, 3, false},
      OpcodeInfo{0x87, "XXX", &CPU::zp, &CPU::XXX, 0, 3, false},
      OpcodeInfo{0x88, "DEY", &CPU::imp, &CPU::DEY, 1, 2, false},
      OpcodeInfo{0x89, "NOP", &CPU::imm, &CPU::NOP, 0, 2, false},
      OpcodeInfo{0x8a, "TXA", &CPU::imp, &CPU::TXA, 1, 2, false},
      OpcodeInfo{0x8b, "XXX", &CPU::imm, &CPU::XXX, 0, 2, false},
      OpcodeInfo{0x8c, "STY", &CPU::abs, &CPU::STY, 3, 4, false},
      OpcodeInfo{0x8d, "STA", &CPU::abs, &CPU::STA, 3, 4, false},
      OpcodeInfo{0x8e, "STX", &CPU::abs, &CPU::STX, 3, 4, false},
      OpcodeInfo{0x8f, "XXX", &CPU::abs, &CPU::XXX, 0, 4, false},
      OpcodeInfo{0x90, "BCC", &CPU::rel, &CPU::BCC, 2, 2, true},
      OpcodeInfo{0x91, "STA", &CPU::indy, &CPU::STA, 2, 6, false},
      OpcodeInfo{0x92, "XXX", &CPU::imp, &CPU::XXX, 0, 2, false},
      OpcodeInfo{0x93, "XXX", &CPU::indy, &CPU::XXX, 0, 6, false},
      OpcodeInfo{0x94, "STY", &CPU::zpx, &CPU::STY, 2, 4, false},
      OpcodeInfo{0x95, "STA", &CPU::zpx, &CPU::STA, 2, 4, false},
      OpcodeInfo{0x96, "STX", &CPU::zpy, &CPU::STX, 2, 4, false},
      OpcodeInfo{0x97, "XXX", &CPU::zpy, &CPU::XXX, 0, 4, false},
      OpcodeInfo{0x98, "TYA", &CPU::imp, &CPU::TYA, 1, 2, false},
      OpcodeInfo{0x99, "STA", &CPU::absy, &CPU::STA, 3, 5, false},
      OpcodeInfo{0x9a, "TXS", &CPU::imp, &CPU::TXS, 1, 2, false},
      OpcodeInfo{0x9b, "XXX", &CPU::absy, &CPU::XXX, 0, 5, false},
      OpcodeInfo{0x9c, "XXX", &CPU::absx, &CPU::XXX, 0, 5, false},
      OpcodeInfo{0x9d, "STA", &CPU::absx, &CPU::STA, 3, 5, false},
      OpcodeInfo{0x9e, "SHX", &CPU::absy, &CPU::XXX, 0, 5, false},
      OpcodeInfo{0x9f, "XXX", &CPU::absy, &CPU::XXX, 0, 5, false},
      OpcodeInfo{0xa0, "LDY", &CPU::imm, &CPU::LDY, 2, 2, false},
      OpcodeInfo{0xa1, "LDA", &CPU::indx, &CPU::LDA, 2, 6, false},
      OpcodeInfo{0xa2, "LDX", &CPU::imm, &CPU::LDX, 2, 2, false},
      OpcodeInfo{0xa3, "XXX", &CPU::indx, &CPU::XXX, 0, 6, false},
      OpcodeInfo{0xa4, "LDY", &CPU::zp, &CPU::LDY, 2, 3, false},
      OpcodeInfo{0xa5, "LDA", &CPU::zp, &CPU::LDA, 2, 3, false},
      OpcodeInfo{0xa6, "LDX", &CPU::zp, &CPU::LDX, 2, 3, false},
      OpcodeInfo{0xa7, "XXX", &CPU::zp, &CPU::XXX, 0, 3, false},
      OpcodeInfo{0xa8, "TAY", &CPU::imp, &CPU::TAY, 1, 2, false},
      OpcodeInfo{0xa9, "LDA", &CPU::imm, &CPU::LDA, 2, 2, false},
      OpcodeInfo{0xaa, "TAX", &CPU::imp, &CPU::TAX, 1, 2, false},
      OpcodeInfo{0xab, "XXX", &CPU::imm, &CPU::XXX, 0, 2, false},
      OpcodeInfo{0xac, "LDY", &CPU::abs, &CPU::LDY, 3, 4, false},
      OpcodeInfo{0xad, "LDA", &CPU::abs, &CPU::LDA, 3, 4, false},
      OpcodeInfo{0xae, "LDX", &CPU::abs, &CPU::LDX, 3, 4, false},
      OpcodeInfo{0xaf, "XXX", &CPU::abs, &CPU::XXX, 0, 4, false},
      OpcodeInfo{0xb0, "BCS", &CPU::rel, &CPU::BCS, 2, 2, true},
      OpcodeInfo{0xb1, "LDA", &CPU::indy, &CPU::LDA, 2, 5, true},
      OpcodeInfo{0xb2, "XXX", &CPU::imp, &CPU::XXX, 0, 2, false},
      OpcodeInfo{0xb3, "XXX", &CPU::indy, &CPU::XXX, 0, 5, true},
      OpcodeInfo{0xb4, "LDY", &CPU::zpx, &CPU::LDY, 2, 4, false},
      OpcodeInfo{0xb5, "LDA", &CPU::zpx, &CPU::LDA, 2, 4, false},
      OpcodeInfo{0xb6, "LDX", &CPU::zpy, &CPU::LDX, 2, 4, false},
      OpcodeInfo{0xb7, "XXX", &CPU::zpy, &CPU::XXX, 0, 4, false},
      OpcodeInfo{0xb8, "CLV", &CPU::imp, &CPU::CLV, 1, 2, false},
      OpcodeInfo{0xb9, "LDA", &CPU::absy, &CPU::LDA, 3, 4, true},
      OpcodeInfo{0xba, "TSX", &CPU::imp, &CPU::TSX, 1, 2, false},
      OpcodeInfo{0xbb, "XXX", &CPU::absy, &CPU::XXX, 0, 4, true},
      OpcodeInfo{0xbc, "LDY", &CPU::absx, &CPU::LDY, 3, 4, true},
      OpcodeInfo{0xbd, "LDA", &CPU::absx, &CPU::LDA, 3, 4, true},
      OpcodeInfo{0xbe, "LDX", &CPU::absy, &CPU::LDX, 3, 4, true},
      OpcodeInfo{0xbf, "XXX", &CPU::absy, &CPU::XXX, 0, 4, true},
      OpcodeInfo{0xc0, "CPY", &CPU::imm, &CPU::CPY, 2, 2, false},
      OpcodeInfo{0xc1, "CMP", &CPU::indx, &CPU::CMP, 2, 6, false},
      OpcodeInfo{0xc2, "NOP", &CPU::imm, &CPU::NOP, 0, 2, false},
      OpcodeInfo{0xc3, "XXX", &CPU::indx, &CPU::XXX, 0, 8, false},
      OpcodeInfo{0xc4, "CPY", &CPU::zp, &CPU::CPY, 2, 3, false},
      OpcodeInfo{0xc5, "CMP", &CPU::zp, &CPU::CMP, 2, 3, false},
      OpcodeInfo{0xc6, "DEC", &CPU::zp, &CPU::DEC, 2, 5, false},
      OpcodeInfo{0xc7, "XXX", &CPU::zp, &CPU::XXX, 0, 5, false},
      OpcodeInfo{0xc8, "INY", &CPU::imp, &CPU::INY, 1, 2, false},
      OpcodeInfo{0xc9, "CMP", &CPU::imm, &CPU::CMP, 2, 2, false},
      OpcodeInfo{0xca, "DEX", &CPU::imp, &CPU::DEX, 1, 2, false},
      OpcodeInfo{0xcb, "XXX", &CPU::imm, &CPU::XXX, 0, 2, false},
      OpcodeInfo{0xcc, "CPY", &CPU::abs, &CPU::CPY, 3, 4, false},
      OpcodeInfo{0xcd, "CMP", &CPU::abs, &CPU::CMP, 3, 4, false},
      OpcodeInfo{0xce, "DEC", &CPU::abs, &CPU::DEC, 3, 6, false},
      OpcodeInfo{0xcf, "XXX", &CPU::abs, &CPU::XXX, 0, 6, false},
      OpcodeInfo{0xd0, "BNE", &CPU::rel, &CPU::BNE, 2, 2, true},
      OpcodeInfo{0xd1, "CMP", &CPU::indy, &CPU::CMP, 2, 5, true},
      OpcodeInfo{0xd2, "XXX", &CPU::imp, &CPU::XXX, 0, 2, false},
      OpcodeInfo{0xd3, "XXX", &CPU::indy, &CPU::XXX, 0, 8, false},
      OpcodeInfo{0xd4, "NOP", &CPU::zpx, &CPU::NOP, 2, 4, false},
      OpcodeInfo{0xd5, "CMP", &CPU::zpx, &CPU::CMP, 2, 4, false},
      OpcodeInfo{0xd6, "DEC", &CPU::zpx, &CPU::DEC, 2, 6, false},
      OpcodeInfo{0xd7, "XXX", &CPU::zpx, &CPU::XXX, 0, 6, false},
      OpcodeInfo{0xd8, "CLD", &CPU::imp, &CPU::CLD, 1, 2, false},
      OpcodeInfo{0xd9, "CMP", &CPU::absy, &CPU::CMP, 3, 4, true},
      OpcodeInfo{0xda, "NOP", &CPU::imp, &CPU::NOP, 1, 2, false},
      OpcodeInfo{0xdb, "XXX", &CPU::absy, &CPU::XXX, 0, 7, false},
      OpcodeInfo{0xdc, "NOP", &CPU::absx, &CPU::NOP, 3, 4, true},
      OpcodeInfo{0xdd, "CMP", &CPU::absx, &CPU::CMP, 3, 4, true},
      OpcodeInfo{0xde, "DEC", &CPU::absx, &CPU::DEC, 3, 7, false},
      OpcodeInfo{0xdf, "XXX", &CPU::absx, &CPU::XXX, 0, 7, false},
      OpcodeInfo{0xe0, "CPX", &CPU::imm, &CPU::CPX, 2, 2, false},
      OpcodeInfo{0xe1, "SBC", &CPU::indx, &CPU::SBC, 2, 6, false},
      OpcodeInfo{0xe2, "NOP", &CPU::imm, &CPU::NOP, 0, 2, false},
      OpcodeInfo{0xe3, "XXX", &CPU::indx, &CPU::XXX, 0, 8, false},
      OpcodeInfo{0xe4, "CPX", &CPU::zp, &CPU::CPX, 2, 3, false},
      OpcodeInfo{0xe5, "SBC", &CPU::zp, &CPU::SBC, 2, 3, false},
      OpcodeInfo{0xe6, "INC", &CPU::zp, &CPU::INC, 2, 5, false},
      OpcodeInfo{0xe7, "XXX", &CPU::zp, &CPU::XXX, 0, 5, false},
      OpcodeInfo{0xe8, "INX", &CPU::imp, &CPU::INX, 1, 2, false},
      OpcodeInfo{0xe9, "SBC", &CPU::imm, &CPU::SBC, 2, 2, false},
      OpcodeInfo{0xea, "NOP", &CPU::imp, &CPU::NOP, 1, 2, false},
      OpcodeInfo{0xeb, "SBC", &CPU::imm, &CPU::SBC, 0, 2, false},
      OpcodeInfo{0xec, "CPX", &CPU::abs, &CPU::CPX, 3, 4, false},
      OpcodeInfo{0xed, "SBC", &CPU::abs, &CPU::SBC, 3, 4, false},
      OpcodeInfo{0xee, "INC", &CPU::abs, &CPU::INC, 3, 6, false},
      OpcodeInfo{0xef, "XXX", &CPU::abs, &CPU::XXX, 0, 6, false},
      OpcodeInfo{0xf0, "BEQ", &CPU::rel, &CPU::BEQ, 2, 2, true},
      OpcodeInfo{0xf1, "SBC", &CPU::indy, &CPU::SBC, 2, 5, true},
      OpcodeInfo{0xf2, "XXX", &CPU::imp, &CPU::XXX, 0, 2, false},
      OpcodeInfo{0xf3, "XXX", &CPU::indy, &CPU::XXX, 0, 8, false},
      OpcodeInfo{0xf4, "NOP", &CPU::zpx, &CPU::NOP, 2, 4, false},
      OpcodeInfo{0xf5, "SBC", &CPU::zpx, &CPU::SBC, 2, 4, false},
      OpcodeInfo{0xf6, "INC", &CPU::zpx, &CPU::INC, 2, 6, false},
      OpcodeInfo{0xf7, "XXX", &CPU::zpx, &CPU::XXX, 0, 6, false},
      OpcodeInfo{0xf8, "SED", &CPU::imp, &CPU::SED, 1, 2, false},
      OpcodeInfo{0xf9, "SBC", &CPU::absy, &CPU::SBC, 3, 4, true},
      OpcodeInfo{0xfa, "NOP", &CPU::imp, &CPU::NOP, 1, 2, false},
      OpcodeInfo{0xfb, "XXX", &CPU::absy, &CPU::XXX, 0, 7, false},
      OpcodeInfo{0xfc, "NOP", &CPU::absx, &CPU::NOP, 3, 4, true},
      OpcodeInfo{0xfd, "SBC", &CPU::absx, &CPU::SBC, 3, 4, true},
      OpcodeInfo{0xfe, "INC", &CPU::absx, &CPU::INC, 3, 7, false},
      OpcodeInfo{0xff, "XXX", &CPU::absx, &CPU::XXX, 0, 7, false},
  };
}

void CPU::branch(bool condition) {
  if (condition) {
    auto page = pc & 0xff00;
    pc = address;
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

void CPU::adc(uint8_t value) {
  uint16_t c = getFlag(Flags::C) ? 1 : 0;
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

void CPU::ADC() {
  uint16_t value = read8();
  adc(value);
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
  auto value = read8();
  setFlag(Flags::Z, (a & value) == 0);
  setFlag(Flags::N, (value & 0x80) != 0);
  setFlag(Flags::V, (value & 0x40) != 0);
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

void CPU::SBC() {
  uint8_t value = ~read8();
  adc(value);
}

void CPU::SEC() { setFlag(Flags::C, true); }

void CPU::SED() { setFlag(Flags::D, true); }

void CPU::SEI() { setFlag(Flags::I, true); }

void CPU::STA() { write8(a); }

void CPU::STX() { write8(x); }

void CPU::STY() { write8(y); }

void CPU::TAX() {
  x = a;
  setZN(a);
}

void CPU::TAY() {
  y = a;
  setZN(a);
}

void CPU::TSX() {
  x = sp;
  setZN(x);
}

void CPU::TXA() {
  a = x;
  setZN(a);
}

void CPU::TXS() { sp = x; }

void CPU::TYA() {
  a = y;
  setZN(a);
}

void CPU::XXX() {}