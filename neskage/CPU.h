#pragma once

#include "types.h"
#include <array>
#include <string_view>
#include <string>

// TODO: revisit this someday, understand the approach more deeply
// this might help: https://stackoverflow.com/a/628079
// 
// this confuses me -- the circular include stuff
// fwd decl, for use as ptr member
class Bus;

class CPU
{
public:
	CPU() {
		// TODO: move this elsewhere
		lookup[0x4C] = { "JMP",&CPU::ABS,&CPU::JMP,3 };
		lookup[0xA2] = { "LDX",&CPU::IMM,&CPU::LDX,2 };
		lookup[0x86] = { "STX",&CPU::ZPG,&CPU::STX,3 };
		lookup[0x20] = { "JSR",&CPU::ABS,&CPU::JSR,6 };
	}
	void connectBus(Bus* b);
	void tick();
	bool instructionDone();
	std::string_view getCurrentInstructionDisasm();
	u64 getTotalCycles() { return totalCycles; }

	/// 6502 state
	u16 pc{ 0xC000 };
	u8 x{ 0 };
	// nestest.log has SP at FD instead of FF (probably some init code im missing)
	u8 sp{ 0xFD };

private:
	/// communication bus
	// ps: this is an aggregate object relation
	// its lifetime is independent of CPU
	Bus* bus{ nullptr };
	u8 read(u16 addr);
	void write(u16 addr, u8 value);

	/// helpers
	int cycles{ 0 };
	u8 opcode{};
	std::string disasm{};
	void populateDisasmString(u16 oldPC);
	// this can overflow but idk how soon
	// also starting at 7 to match nestest log output
	// probably starts at 7 because of some reset vector thingy
	u64 totalCycles{ 7 };

	/// addressing modes
	// these functions fetch data into addr variables for execution
	u16 addrAbs{};
	int ABS();
	int IMM();
	int ZPG();

	/// execution
	// the actual logic
	int JMP();
	int LDX();
	int STX();
	int JSR();

	/// lookup table
	using addrModeFunc = int(CPU::*)();
	using operateFunc = int(CPU::*)();
	struct Instruction {
		std::string_view name{ "???" };
		addrModeFunc addrMode{ nullptr };
		operateFunc operate{ nullptr };
		int cycles{ 0 };
	};
	std::array<Instruction, 256> lookup{};
};