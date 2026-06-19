#include "CPU.h"

#include "Bus.h"

#include <format>

void CPU::connectBus(Bus* b) {
	bus = b;
}

void CPU::tick() {
	if (cycles == 0) {
		populateRegisterStateString();
		populateTotalCyclesString();

		u16 prevPC = pc;
		opcode = read(pc++);

		cycles = lookup[opcode].cycles;
		// kinda ugly syntax, but binds "this" to the member func call, then calls it
		// there's also std::invoke(lookup[opcode].addrMode, this)
		cycles += (this->*lookup[opcode].addrMode)();
		cycles += (this->*lookup[opcode].operate)();

		// reusing work done by addrMode by doing this in the end
		populateDisasmString(prevPC);
	}

	--cycles;
	++totalCycles;
}

bool CPU::instructionDone() {
	return cycles == 0;
}

void CPU::setFlag(StatusFlag f, bool v)
{
	// branchless could be possible
	if (v) {
		status = (status & ~f) | f;
	}
	else {
		status = (status & ~f);
	}
}

bool CPU::getFlag(StatusFlag f)
{
	return (status >> f) & 1;
}

std::string_view CPU::dumpCurrentInstructionInfo() {
	dumpInfo = disasm + registerState + " " + totalCyclesString;
	return dumpInfo;
}

// TODO: better column output, manual tabbing is painful
void CPU::populateDisasmString(u16 prevPC) {
	disasm = std::format("{:04X}  {:02X} ", prevPC, read(prevPC)); // PC  OP
	if (lookup[opcode].addrMode == &CPU::ABS) {
		disasm += std::format("{:02X} {:02X}\t", read(prevPC + 1), read(prevPC + 2)); // LL HH
		disasm += lookup[opcode].name; // NAME
		disasm += std::format(" ${:04X}", addr); // HHLL
	}

	if (lookup[opcode].addrMode == &CPU::IMM) {
		disasm += std::format("{:02X}\t", read(prevPC + 1)); // BB
		disasm += lookup[opcode].name; // NAME
		disasm += std::format(" #${:02X}", read(addr)); // BB
	}

	if (lookup[opcode].addrMode == &CPU::ZPG) {
		disasm += std::format("{:02X}\t", read(prevPC + 1)); // LL
		disasm += lookup[opcode].name; // NAME
		disasm += std::format(" ${:02X}", addr); // LL
		if (lookup[opcode].name == "STX") {
			disasm += std::format(" = {:02X}", read(addr));
		}
	}

	if (lookup[opcode].addrMode == &CPU::IMP) {
		disasm += "\t";
		disasm += lookup[opcode].name; // NAME
		disasm += "\t";
	}

	if (lookup[opcode].addrMode == &CPU::REL) {
		disasm += std::format("{:02X}\t", read(prevPC + 1)); // BB
		disasm += lookup[opcode].name; // NAME
		disasm += std::format(" ${:04X}", addr); // HHLL
	}

	disasm += "\t";
}

void CPU::populateRegisterStateString()
{
	registerState = std::format("X:{:02X} P:{:02X} SP:{:02X}", x, status, sp);
}

void CPU::populateTotalCyclesString()
{
	totalCyclesString = std::format("CYC:{}", totalCycles);
}

bool CPU::pageCrossed(u16 oldAddr, u16 newAddr)
{
	return ((oldAddr >> 8) & 0xff) != ((newAddr >> 8) & 0xff);
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
	addr = (hi << 8) | lo;
	return 0;
}

int CPU::IMM() {
	addr = pc++;
	return 0;
}

int CPU::ZPG() {
	addr = read(pc++);
	return 0;
}

int CPU::IMP() {
	return 0;
}

int CPU::REL() {
	u16 addrPrev = addr;
	i8 rel = read(pc++);
	addr = pc + rel;

	if (pageCrossed(addrPrev, addr)) {
		return 1;
	}

	return 0;
}

/// execution

int CPU::JMP() {
	pc = addr;
	return 0;
}

int CPU::LDX() {
	x = read(addr);
	setFlag(Z, x == 0);
	setFlag(N, (x >> 7) & 1);
	return 0;
}

int CPU::STX() {
	write(addr, x);
	return 0;
}

int CPU::JSR() {
	u8 pcHH = (pc >> 8) & 0xFF;
	u8 pcLL = pc & 0xFF;

	// not checking any overflow/underflow yet
	write(sp--, pcHH);
	write(sp--, pcLL);

	pc = addr;
	return 0;
}

int CPU::NOP() {
	return 0;
}

int CPU::SEC() {
	setFlag(C, 1);
	return 0;
}

int CPU::BCS() {
	if (getFlag(C)) {
		pc = addr;
		return 1;
	}
	return 0;
}