#include "CPU.h"

#include "Bus.h"

#include <format>

void CPU::connectBus(Bus* b) {
	bus = b;
}

void CPU::tick() {
	if (cycles == 0) {
		u16 prevPC = pc;
		opcode = read(pc++);

		cycles = lookup[opcode].cycles;
		// kinda ugly syntax, but binds "this" to the member func call, then calls it
		// there's also std::invoke(lookup[opcode].addrMode, this)
		(this->*lookup[opcode].addrMode)();
		(this->*lookup[opcode].operate)();

		populateDisasmString(prevPC);
	}

	--cycles;
	++totalCycles;
}

bool CPU::instructionDone() {
	return cycles == 0;
}

std::string_view CPU::getCurrentInstructionDisasm() {
	return disasm;
}

void CPU::populateDisasmString(u16 prevPC) {
	disasm = "";
	disasm += std::format("{:04X}  ", prevPC);
	if (lookup[opcode].addrMode == &CPU::ABS) {
		disasm += std::format("{:02X} {:02X} {:02X}\t", read(prevPC), read(prevPC + 1), read(prevPC + 2));
		disasm += lookup[opcode].name;
		disasm += std::format(" ${:04X}\t", addrAbs);
	}

	if (lookup[opcode].addrMode == &CPU::IMM) {
		disasm += std::format("{:02X} {:02X}\t", read(prevPC), read(prevPC + 1));
		disasm += lookup[opcode].name;
		disasm += std::format(" #${:02X}\t", read(addrAbs));
	}

	disasm += std::format("X:{:02X} CYC:{}", x, totalCycles);
}

u8 CPU::read(u16 addr) {
	return bus->read(addr);
}

void CPU::write(u16 addr, u8 value) {
	bus->write(addr, value);
}

/// addressing modes

int CPU::ABS() {
	u8 lo = read(pc++);
	u8 hi = read(pc++);
	addrAbs = (hi << 8) | lo;
	return 0;
}

int CPU::IMM() {
	addrAbs = pc++;
	return 0;
}

int CPU::ZPG() {
	addrAbs = read(pc++);
	return 0;
}

/// execution

int CPU::JMP() {
	pc = addrAbs;
	return 0;
}

int CPU::LDX() {
	x = read(addrAbs);
	return 0;
}

int CPU::STX() {
	write(addrAbs, x);
	return 0;
}