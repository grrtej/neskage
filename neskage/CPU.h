#pragma once

#include "types.h"
#include <array>
#include <string>
#include <string_view>

/// THANK YOU KEVTRIS FOR NESTEST !!

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
		lookup[0xAE] = { "LDX",&CPU::ABS,&CPU::LDX,4 };
		lookup[0xA0] = { "LDY",&CPU::IMM,&CPU::LDY,2 };
		lookup[0x29] = { "AND",&CPU::IMM,&CPU::AND,2 };
		lookup[0x49] = { "EOR",&CPU::IMM,&CPU::EOR,2 };
		lookup[0x69] = { "ADC",&CPU::IMM,&CPU::ADC,2 };
		lookup[0xE9] = { "SBC",&CPU::IMM,&CPU::SBC,2 };
		lookup[0x4A] = { "LSR A",&CPU::IMP,&CPU::LSR,2 };
		lookup[0x0A] = { "ASL A",&CPU::IMP,&CPU::ASL,2 };
		lookup[0x6A] = { "ROR A",&CPU::IMP,&CPU::ROR,2 };
		lookup[0x2A] = { "ROL A",&CPU::IMP,&CPU::ROL,2 };
		lookup[0xC9] = { "CMP",&CPU::IMM,&CPU::CMP,2 };
		lookup[0xC0] = { "CPY",&CPU::IMM,&CPU::CPY,2 };
		lookup[0xE0] = { "CPX",&CPU::IMM,&CPU::CPX,2 };
		lookup[0x85] = { "STA",&CPU::ZPG,&CPU::STA,3 };
		lookup[0x86] = { "STX",&CPU::ZPG,&CPU::STX,3 };
		lookup[0x8E] = { "STX",&CPU::ABS,&CPU::STX,4 };
		lookup[0x20] = { "JSR",&CPU::ABS,&CPU::JSR,6 };
		lookup[0x60] = { "RTS",&CPU::IMP,&CPU::RTS,6 };
		lookup[0x40] = { "RTI",&CPU::IMP,&CPU::RTI,6 };
		lookup[0xEA] = { "NOP",&CPU::IMP,&CPU::NOP,2 };
		lookup[0x38] = { "SEC",&CPU::IMP,&CPU::SEC,2 };
		lookup[0x18] = { "CLC",&CPU::IMP,&CPU::CLC,2 };
		lookup[0xD8] = { "CLD",&CPU::IMP,&CPU::CLD,2 };
		lookup[0x78] = { "SEI",&CPU::IMP,&CPU::SEI,2 };
		lookup[0xF8] = { "SED",&CPU::IMP,&CPU::SED,2 };
		lookup[0x68] = { "PLA",&CPU::IMP,&CPU::PLA,4 };
		lookup[0x08] = { "PHP",&CPU::IMP,&CPU::PHP,3 };
		lookup[0x28] = { "PLP",&CPU::IMP,&CPU::PLP,4 };
		lookup[0x48] = { "PHA",&CPU::IMP,&CPU::PHA,3 };
		lookup[0x10] = { "BPL",&CPU::REL,&CPU::BPL,2 };
		lookup[0x30] = { "BMI",&CPU::REL,&CPU::BMI,2 };
		lookup[0x50] = { "BVC",&CPU::REL,&CPU::BVC,2 };
		lookup[0x70] = { "BVS",&CPU::REL,&CPU::BVS,2 };
		lookup[0x90] = { "BCC",&CPU::REL,&CPU::BCC,2 };
		lookup[0xB0] = { "BCS",&CPU::REL,&CPU::BCS,2 };
		lookup[0xA9] = { "LDA",&CPU::IMM,&CPU::LDA,2 };
		lookup[0xAD] = { "LDA",&CPU::ABS,&CPU::LDA,4 };
		lookup[0xA5] = { "LDA",&CPU::ZPG,&CPU::LDA,3 };
		lookup[0xF0] = { "BEQ",&CPU::REL,&CPU::BEQ,2 };
		lookup[0xD0] = { "BNE",&CPU::REL,&CPU::BNE,2 };
		lookup[0x24] = { "BIT",&CPU::ZPG,&CPU::BIT,3 };
		lookup[0x09] = { "ORA",&CPU::IMM,&CPU::ORA,2 };
		lookup[0xB8] = { "CLV",&CPU::IMP,&CPU::CLV,2 };
		lookup[0xC8] = { "INY",&CPU::IMP,&CPU::INY,2 };
		lookup[0xE8] = { "INX",&CPU::IMP,&CPU::INX,2 };
		lookup[0x88] = { "DEY",&CPU::IMP,&CPU::DEY,2 };
		lookup[0xCA] = { "DEX",&CPU::IMP,&CPU::DEX,2 };
		lookup[0xA8] = { "TAY",&CPU::IMP,&CPU::TAY,2 };
		lookup[0xAA] = { "TAX",&CPU::IMP,&CPU::TAX,2 };
		lookup[0x98] = { "TYA",&CPU::IMP,&CPU::TYA,2 };
		lookup[0x8A] = { "TXA",&CPU::IMP,&CPU::TXA,2 };
		lookup[0xBA] = { "TSX",&CPU::IMP,&CPU::TSX,2 };
		lookup[0x9A] = { "TXS",&CPU::IMP,&CPU::TXS,2 };
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
	u8 y{ 0 };
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

	// TODO: check how javidx9 does this, delayed interrupt flag setting
	// i just keep throwing more variables at a problem, clutter !!
	bool setFlagIRequested{ false };
	bool setFlagIValue{};
	void setFlagIDelayed(bool v) { setFlagIRequested = true; setFlagIValue = v; }

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
	int LDY();
	int STA();
	int STX();
	int NOP();
	int SEC();
	int CLC();
	int CLD();
	int LDA();
	int BIT();
	int SEI();
	int SED();
	int PHP();
	int PLP();
	int PLA();
	int PHA();
	int AND();
	int CMP();
	int CPY();
	int CPX();
	int ORA();
	int CLV();
	int EOR();
	int ADC();
	int SBC();
	int LSR();
	int ASL();
	int ROR();
	int ROL();
	int INY();
	int INX();
	int DEY();
	int DEX();
	int TAY();
	int TAX();
	int TYA();
	int TXA();
	int TSX();
	int TXS();

	// the stack type is empty (stack pointer points at next empty cell)
	// and descending (grows downwards)
	// the $0100 - $01FF page is for the stack
	// not checking any stack overflow/underflow yet
	u8 stackPop() { return read(0x100 + ++sp); }
	void stackPush(u8 v) { write(0x100 + sp--, v); }
	int JSR();
	int RTS();
	int RTI();

	int branchIf(StatusFlag f, bool cond);
	int BPL() { return branchIf(N, false); }
	int BMI() { return branchIf(N, true); }
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