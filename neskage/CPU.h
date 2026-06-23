#pragma once

#include "types.h"
#include <array>
#include <string>
#include <string_view>

// TODO: revisit this someday, understand the approach more deeply
// this might help: https://stackoverflow.com/a/628079
// 
// this confuses me -- the circular include stuff
// fwd decl, for use as ptr member
class Bus;

/*
* 6502 interpreter core
*
* this is starting to do too much. ideally it should be a pure state machine interpreting 6502 machine code.
* try to delegate disassembly to another unit.
*/
class CPU
{
public:
	CPU() {
		// TODO: move this elsewhere
		lookup[0x4C] = { "JMP",&CPU::ABS,&CPU::JMP,3 };
		lookup[0xA2] = { "LDX",&CPU::IMM,&CPU::LDX,2 };
		lookup[0x85] = { "STA",&CPU::ZPG,&CPU::STA,3 };
		lookup[0x86] = { "STX",&CPU::ZPG,&CPU::STX,3 };
		lookup[0x20] = { "JSR",&CPU::ABS,&CPU::JSR,6 };
		lookup[0x60] = { "RTS",&CPU::IMP,&CPU::RTS,6 };
		lookup[0xEA] = { "NOP",&CPU::IMP,&CPU::NOP,2 };
		lookup[0x38] = { "SEC",&CPU::IMP,&CPU::SEC,2 };
		lookup[0x18] = { "CLC",&CPU::IMP,&CPU::CLC,2 };
		lookup[0x78] = { "SEI",&CPU::IMP,&CPU::SEI,2 };
		lookup[0xF8] = { "SED",&CPU::IMP,&CPU::SED,2 };
		lookup[0x10] = { "BPL",&CPU::REL,&CPU::BPL,2 };
		lookup[0x50] = { "BVS",&CPU::REL,&CPU::BVC,2 };
		lookup[0x70] = { "BVS",&CPU::REL,&CPU::BVS,2 };
		lookup[0x90] = { "BCC",&CPU::REL,&CPU::BCC,2 };
		lookup[0xB0] = { "BCS",&CPU::REL,&CPU::BCS,2 };
		lookup[0xA9] = { "LDA",&CPU::IMM,&CPU::LDA,2 };
		lookup[0xF0] = { "BEQ",&CPU::REL,&CPU::BEQ,2 };
		lookup[0xD0] = { "BNE",&CPU::REL,&CPU::BNE,2 };
		lookup[0x24] = { "BIT",&CPU::ZPG,&CPU::BIT,3 };
	}
	void connectBus(Bus* b);
	void tick();
	bool instructionDone();
	// read only getter, im sure there is some c++ keyword to show my intent
	u64 getTotalCycleCount() { return totalCycles; }
	std::string_view dumpCurrentInstructionInfo();

	/// 6502 state
	u16 pc{ 0xC000 };
	u8 a{ 0 };
	u8 x{ 0 };
	// nestest.log has SP at FD instead of FF (probably some init code im missing)
	u8 sp{ 0xFD };
	u8 status{ 0x00 | (U | I) };

private:
	/// communication bus
	// ps: this is an aggregate object relation
	// its lifetime is independent of CPU
	Bus* bus{ nullptr };
	u8 read(u16 addr);
	void write(u16 addr, u8 value);

	/// helpers
	u8 opcode{};
	int cycles{ 0 };
	// this can overflow but idk how soon
	// also starting at 7 to match nestest log output
	// probably starts at 7 because of some reset vector thingy
	u64 totalCycles{ 7 };

	std::string disasm{};
	std::string registerState{};
	std::string totalCyclesString{};
	std::string dumpInfo{};
	void populateDisasmString(u16 oldPC);
	void populateRegisterStateString();
	void populateTotalCyclesString();

	enum StatusFlag {
		C = (1 << 0), // carry
		Z = (1 << 1), // zero
		I = (1 << 2), // interrupt disable
		D = (1 << 3), // decimal (nes doesnt use this?)
		B = (1 << 4), // b flag
		U = (1 << 5), // unused, always 1
		V = (1 << 6), // overflow
		N = (1 << 7), // negative
	};
	void setFlag(StatusFlag f, bool v);
	bool getFlag(StatusFlag f);
	bool seiRequested{ false };

	/// addressing modes
	// these functions fetch data into addr variables for execution
	u16 addr{};
	int ABS();
	int IMM();
	int ZPG();
	int IMP();
	int REL();

	/// execution
	// the actual logic
	int JMP();
	int LDX();
	int STA();
	int STX();
	int NOP();
	int SEC();
	int CLC();
	int LDA();
	int BIT();
	int SEI();
	int SED();

	// the stack type is empty (stack pointer points at next empty cell)
	// and descending (grows upwards)
	// not checking any stack overflow/underflow yet
	u8 stackPop() { return read(++sp); }
	void stackPush(u8 v) { write(sp--, v); }
	int JSR();
	int RTS();

	int branchIf(StatusFlag f, bool cond);
	int BPL() { return branchIf(N, false); }
	int BVS() { return branchIf(V, true); }
	int BVC() { return branchIf(V, false); }
	int BCS() { return branchIf(C, true); }
	int BCC() { return branchIf(C, false); }
	int BEQ() { return branchIf(Z, true); }
	int BNE() { return branchIf(Z, false); }

	/// lookup table
	using addrModeFunc = int(CPU::*)();
	using operateFunc = int(CPU::*)();
	struct Instruction {
		std::string_view name{ "???" };
		// maybe some fallback func instead of null
		addrModeFunc addrMode{ nullptr };
		operateFunc operate{ nullptr };
		int cycles{ 0 };
	};
	std::array<Instruction, 256> lookup{};
};