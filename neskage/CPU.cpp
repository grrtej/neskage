#include "CPU.h"

#include "Bus.h"

#include <cassert>

static bool pageCrossed(u16 oldAddr, u16 newAddr);

void CPU::connectBus(Bus* b) {
	bus = b;
}

void CPU::tick() {
	if (cycles == 0) {
		// here interrupt flag is set BEFORE next instruction executes but after SEI etc is done
		// could be wrong ?
		if (setFlagIRequested) {
			setFlagIRequested = false;
			setFlag(I, setFlagIValue);
		}

		u16 prevPC = pc;
		opcode = read(pc++);

		cycles = lookup[opcode].cycles;
		(this->*lookup[opcode].addrMode)();
		if (disasmEventHandler) {
			populateDisasmEventData(prevPC);
			disasmEventHandler(disasmEventData);
		}
		(this->*lookup[opcode].operate)();
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
	return 0;
}

int CPU::IND_X() {
	u8 ptrZpg = read(pc++) + x;
	u8 addrLL = read(ptrZpg % 256);
	u8 addrHH = read((ptrZpg + 1) % 256); // note that u8+int literal = int
	addr = (addrHH << 8) | addrLL;
	return 0;
}

/// execution

// TODO: setFlag Z and N are repeated a lot, DRY (macro? or some modern c++ thingy?)

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

int CPU::LDY() {
	y = read(addr);
	setFlag(Z, y == 0);
	setFlag(N, (y >> 7) & 1);
	return 0;
}

int CPU::AND() {
	a &= read(addr);
	setFlag(Z, a == 0);
	setFlag(N, (a >> 7) & 1);
	return 0;
}

int CPU::CMP() {
	u8 v = read(addr);
	u8 res = a - v;

	// C is NO BORROW flag here
	// imagine: C is used as fuel for borrowing
	// when used up, it becomes 0
	// when unused, it stays 1
	setFlag(C, a >= v);

	setFlag(Z, a == v);
	setFlag(N, (res >> 7) & 1);

	return 0;
}

int CPU::CPY() {
	u8 v = read(addr);
	u8 res = y - v;

	setFlag(C, y >= v);
	setFlag(Z, y == v);
	setFlag(N, (res >> 7) & 1);

	return 0;
}

int CPU::CPX() {
	u8 v = read(addr);
	u8 res = x - v;

	setFlag(C, x >= v);
	setFlag(Z, x == v);
	setFlag(N, (res >> 7) & 1);

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

int CPU::STY() {
	write(addr, y);
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

int CPU::RTI() {
	status = stackPop() | 0b00100000;

	u8 pcLL = stackPop();
	u8 pcHH = stackPop();
	pc = (pcHH << 8) | pcLL;

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

int CPU::CLD() {
	setFlag(D, 0);
	return 0;
}

int CPU::CLV() {
	setFlag(V, 0);
	return 0;
}

int CPU::SEI() {
	setFlagIDelayed(true);
	return 0;
}

int CPU::SED() {
	setFlag(D, true);
	return 0;
}

int CPU::PHP() {
	stackPush(status | 0b00110000);
	return 0;
}

int CPU::PLP() {
	u8 popped = stackPop();
	// TODO: explain a lil (filtering and merging of parts)
	status = (status & 0x34) | (popped & 0xCB);
	setFlagIDelayed((popped >> 2) & 1);
	return 0;
}

int CPU::PLA() {
	a = stackPop();
	setFlag(Z, a == 0);
	setFlag(N, (a >> 7) & 1);
	return 0;
}

int CPU::PHA() {
	stackPush(a);
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
	u8 v = read(addr);
	setFlag(Z, (a & v) == 0);
	setFlag(V, (v >> 6) & 1);
	setFlag(N, (v >> 7) & 1);
	return 0;
}

int CPU::ORA() {
	a |= read(addr);
	setFlag(Z, a == 0);
	setFlag(N, (a >> 7) & 1);
	return 0;
}

int CPU::EOR() {
	a ^= read(addr);
	setFlag(Z, a == 0);
	setFlag(N, (a >> 7) & 1);
	return 0;
}

int CPU::ADC() {
	u8 v = read(addr);
	u16 res = a + v + getFlag(C);

	setFlag(C, res > 0xFF);
	// overflow: sign of result is different from the sign of both operands (a, v)
	// when pos + pos = neg -> overflow !!
	// when neg + neg = pos -> overflow !!
	// but pos + neg can never overflow
	setFlag(V, (res ^ a) & (res ^ v) & 0x80);

	a = res & 0xFF;
	setFlag(Z, a == 0);
	setFlag(N, (a >> 7) & 1);

	return 0;
}

// can i reuse ADC ? maybe, but dont destroy readability
int CPU::SBC() {
	u8 v = read(addr);
	u16 res = a + (~v & 0xFF) + getFlag(C);

	setFlag(C, res > 0xFF);
	// was reading 6502 asm guide by leventhal,
	// overflow is set to xor of carry-in and carry-out of bit 7.
	// i assume the actual hardware is literally wired like that.
	// this shortcut works too:
	setFlag(V, (res ^ a) & (res ^ ~v) & 0x80);

	a = res & 0xFF;
	setFlag(Z, a == 0);
	setFlag(N, (a >> 7) & 1);

	return 0;
}

int CPU::LSR() {
	setFlag(C, a & 1);

	a >>= 1;
	setFlag(Z, a == 0);
	setFlag(N, false); // bit 7 was zero filled during shift, so no need to check

	return 0;
}

int CPU::ASL() {
	setFlag(C, (a >> 7) & 1);

	a <<= 1;
	setFlag(Z, a == 0);
	setFlag(N, (a >> 7) & 1);

	return 0;
}

int CPU::ROR() {
	u8 currentC = getFlag(C);
	bool newC = a & 1;

	a = (a >> 1) | (currentC << 7);
	setFlag(C, newC);

	setFlag(Z, a == 0);
	setFlag(N, (a >> 7) & 1);

	return 0;
}

int CPU::ROL() {
	u8 currentC = getFlag(C);
	bool newC = (a >> 7) & 1;

	a = (a << 1) | currentC;
	setFlag(C, newC);

	setFlag(Z, a == 0);
	setFlag(N, (a >> 7) & 1);

	return 0;
}

int CPU::INY() {
	y++;

	setFlag(Z, y == 0);
	setFlag(N, (y >> 7) & 1);

	return 0;
}

int CPU::INX() {
	x++;

	setFlag(Z, x == 0);
	setFlag(N, (x >> 7) & 1);

	return 0;
}

int CPU::DEY() {
	--y;

	setFlag(Z, y == 0);
	setFlag(N, (y >> 7) & 1);

	return 0;
}

int CPU::DEX() {
	--x;

	setFlag(Z, x == 0);
	setFlag(N, (x >> 7) & 1);

	return 0;
}

int CPU::TAY() {
	y = a;

	setFlag(Z, y == 0);
	setFlag(N, (y >> 7) & 1);

	return 0;
}

int CPU::TAX() {
	x = a;

	setFlag(Z, x == 0);
	setFlag(N, (x >> 7) & 1);

	return 0;
}

int CPU::TYA() {
	a = y;

	setFlag(Z, a == 0);
	setFlag(N, (a >> 7) & 1);

	return 0;
}

int CPU::TXA() {
	a = x;

	setFlag(Z, a == 0);
	setFlag(N, (a >> 7) & 1);

	return 0;
}

int CPU::TSX() {
	x = sp;

	setFlag(Z, x == 0);
	setFlag(N, (x >> 7) & 1);

	return 0;
}

int CPU::TXS() {
	sp = x;
	// no status flags affected
	return 0;
}

/// helpers

int CPU::branchIf(StatusFlag f, bool cond) {
	if (getFlag(f) == cond) {
		cycles++;
		if (pageCrossed(pc, addr)) {
			cycles++;
		}
		pc = addr;
	}
	return 0;
}

// i think copying/duplicating this data is worth it for now
void CPU::populateDisasmEventData(u16 prevPC) {
	disasmEventData.a = a;
	disasmEventData.x = x;
	disasmEventData.y = y;
	disasmEventData.status = status;
	disasmEventData.sp = sp;
	disasmEventData.pc = prevPC;
	disasmEventData.totalCycles = totalCycles;
	disasmEventData.addr = addr;
	disasmEventData.memVal = read(addr);
	disasmEventData.name = lookup[opcode].name;

	disasmEventData.ops.clear();
	for (int i = 0; i < pc - prevPC; i++) {
		disasmEventData.ops.push_back(read(prevPC + i));
	}
	assert(disasmEventData.ops.size() <= 3);

	// map addrModeFunc -> Disasm addrMode enum
	if (lookup[opcode].addrMode == &CPU::ABS) {
		disasmEventData.addrMode = DisasmEvent::ABS;
	}
	else if (lookup[opcode].addrMode == &CPU::IMM) {
		disasmEventData.addrMode = DisasmEvent::IMM;
	}
	else if (lookup[opcode].addrMode == &CPU::ZPG) {
		disasmEventData.addrMode = DisasmEvent::ZPG;
	}
	else if (lookup[opcode].addrMode == &CPU::IMP) {
		disasmEventData.addrMode = DisasmEvent::IMP;
	}
	else if (lookup[opcode].addrMode == &CPU::REL) {
		disasmEventData.addrMode = DisasmEvent::REL;
	}
	else if (lookup[opcode].addrMode == &CPU::IND_X) {
		disasmEventData.addrMode = DisasmEvent::IND_X;
	}
}

// TODO: https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#sf22-use-an-unnamed-anonymous-namespace-for-all-internalnon-exported-entities
// rationale: https://www.learncpp.com/cpp-tutorial/unnamed-and-inline-namespaces/#:~:text=Copy-,Unnamed,static%2E
static bool pageCrossed(u16 oldAddr, u16 newAddr) {
	// compare HI bytes
	return (oldAddr & 0xFF00) != (newAddr & 0xFF00);
}