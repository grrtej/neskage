#pragma once

#include "types.h"
#include <array>
#include <string_view>

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
		// just one for now
		lookup[0x4C] = { "JMP",&CPU::ABS,&CPU::JMP,3 };
	}
	void connectBus(Bus* b);
	void tick();

private:
	/// communication bus
	// ps: this is an aggregate object relation
	// its lifetime is independent of CPU
	Bus* bus{ nullptr };
	u8 read(u16 addr);
	void write(u16 addr, u8 value);

	/// 6502 state
	u16 pc{ 0xC000 };

	/// helpers
	int cycles{ 0 };
	u8 opcode{};

	/// addressing modes
	// these functions fetch data into addr variables for execution
	u16 addrAbs{};
	int ABS();

	/// execution
	// the actual logic
	int JMP();

	// lookup table
	using addrModeFunc = int(CPU::*)();
	using operateFunc = int(CPU::*)();
	struct Instruction {
		std::string_view name{ "???" };
		addrModeFunc addrMode{ nullptr };
		operateFunc operate{ nullptr };
		int cycles{0};
	};
	std::array<Instruction, 256> lookup{};
};