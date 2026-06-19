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
		lookup[0xEA] = { "NOP",&CPU::IMP,&CPU::NOP,2 };
		lookup[0x38] = { "SEC",&CPU::IMP,&CPU::SEC,2 };
		lookup[0xB0] = { "BCS",&CPU::REL,&CPU::BCS,2 };
	}
	void connectBus(Bus* b);
	void tick();
	bool instructionDone();
	std::string_view dumpCurrentInstructionInfo();

	/// 6502 state
	u16 pc{ 0xC000 };
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
	void populateRegisterStateString();
	void populateDisasmString(u16 oldPC);
	void populateTotalCyclesString();
	bool pageCrossed(u16 oldAddr, u16 newAddr);

	enum StatusFlag {
		C = (1 << 0),
		Z = (1 << 1),
		I = (1 << 2),
		D = (1 << 3),
		B = (1 << 4),
		U = (1 << 5),
		V = (1 << 6),
		N = (1 << 7),
	};
	void setFlag(StatusFlag f, bool v);
	bool getFlag(StatusFlag f);

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
	int STX();
	int JSR();
	int NOP();
	int SEC();
	int BCS();

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