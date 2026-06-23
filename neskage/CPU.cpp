#include "CPU.h"

#include "Bus.h"

#include <format>

static bool pageCrossed(u16 oldAddr, u16 newAddr);

void CPU::connectBus(Bus* b) {
	bus = b;
}

void CPU::tick() {
	if (cycles == 0) {
		populateRegisterStateString();
		populateTotalCyclesString();

		// here interrupt flag is set BEFORE next instruction executes but after SEI is done
		// could be wrong ?
		if (seiRequested) {
			seiRequested = false;
			setFlag(I, true);
		}

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
	// f is the bit location (0 .. 7)
	//
	// (status & ~f): clears status bit at f
	//
	// !! converts bool to a number
	// (-v & f): creates a number with bit at f set to v
	//   -v: two's complement of v (~v + 1, ignore overflow)
	//     if v = true (1), -v = 0b11111111
	//     if v = false (0), -v = 0b00000000
	//   for f = D (0b00001000):
	//     if v = true, (-v & f) = 0b00001000
	//     if v = false, (-v & f) = 0b00000000
	status = (status & ~f) | (-!!v & f);
}

bool CPU::getFlag(StatusFlag f)
{
	return (status & f) > 0;
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
		// prefix compare or something?
		if (lookup[opcode].name == "STX"
			|| lookup[opcode].name == "STA"
			|| lookup[opcode].name == "BIT") {
			// i think nestest.log shows prev value of the memory location
			// so this WILL NOT MATCH the output
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
	registerState = std::format("A:{:02X} X:{:02X} P:{:02X} SP:{:02X}", a, x, status, sp);
}

void CPU::populateTotalCyclesString()
{
	totalCyclesString = std::format("CYC:{}", totalCycles);
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
	i8 rel = read(pc++);
	addr = pc + rel;

	// right now pc contains the next instruction address if branch was not taken.
	// but if branch is taken, pc will contain addr. do these two cases have different pages?
	// if yes, page crossed.
	// this is my understanding
	if (pageCrossed(pc, addr)) {
		return 1; // extra cycle
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

int CPU::STA() {
	write(addr, a);
	return 0;
}

int CPU::STX() {
	write(addr, x);
	return 0;
}

int CPU::JSR() {
	--pc; // because RTS does ++pc after pop

	u8 pcHH = (pc >> 8) & 0xFF;
	u8 pcLL = pc & 0xFF;
	stackPush(pcHH);
	stackPush(pcLL);

	pc = addr;
	return 0;
}

int CPU::RTS() {
	u8 pcLL = stackPop();
	u8 pcHH = stackPop();

	pc = (pcHH << 8) | pcLL;
	++pc;
	return 0;
}

int CPU::NOP() {
	return 0;
}

int CPU::SEC() {
	setFlag(C, 1);
	return 0;
}

int CPU::CLC() {
	setFlag(C, 0);
	return 0;
}

int CPU::SEI() {
	seiRequested = true;
	return 0;
}

int CPU::SED() {
	setFlag(D, true);
	return 0;
}

// all these LD* operations are pretty similar...
int CPU::LDA() {
	a = read(addr);
	setFlag(Z, a == 0);
	setFlag(N, (a >> 7) & 1);
	return 0;
}

int CPU::BIT() {
	setFlag(Z, (a & read(addr)) == 0);
	setFlag(V, (a >> 6) & 1);
	setFlag(N, (a >> 7) & 1);
	return 0;
}

/// helpers

int CPU::branchIf(StatusFlag f, bool cond) {
	if (getFlag(f) == cond) {
		pc = addr;
		return 1; // branch taken, extra cycle
	}
	return 0;
}

static bool pageCrossed(u16 oldAddr, u16 newAddr) {
	// compare HI bytes
	return ((oldAddr >> 8) & 0xff) != ((newAddr >> 8) & 0xff);
}