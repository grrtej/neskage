#pragma once

#include "types.h"
#include "event.h"
#include <array>
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
* TODO: organize for readability, prune stuff not needed
*/
class CPU
{
public:
	CPU() {
		// TODO: move this elsewhere
		lookup[0x4C] = { "JMP",&CPU::ABS,&CPU::JMP,3 };
		lookup[0xA2] = { "LDX",&CPU::IMM,&CPU::LDX,2 };
		lookup[0xAE] = { "LDX",&CPU::ABS,&CPU::LDX,4 };
		lookup[0xA6] = { "LDX",&CPU::ZPG,&CPU::LDX,3 };
		lookup[0xA0] = { "LDY",&CPU::IMM,&CPU::LDY,2 };
		lookup[0xA4] = { "LDY",&CPU::ZPG,&CPU::LDY,3 };
		lookup[0xAC] = { "LDY",&CPU::ABS,&CPU::LDY,4 };
		lookup[0x29] = { "AND",&CPU::IMM,&CPU::AND,2 };
		lookup[0x25] = { "AND",&CPU::ZPG,&CPU::AND,3 };
		lookup[0x21] = { "AND",&CPU::IND_X,&CPU::AND,6 };
		lookup[0x2D] = { "AND",&CPU::ABS,&CPU::AND,4 };
		lookup[0x49] = { "EOR",&CPU::IMM,&CPU::EOR,2 };
		lookup[0x45] = { "EOR",&CPU::ZPG,&CPU::EOR,3 };
		lookup[0x41] = { "EOR",&CPU::IND_X,&CPU::EOR,6 };
		lookup[0x4D] = { "EOR",&CPU::ABS,&CPU::EOR,4 };
		lookup[0x69] = { "ADC",&CPU::IMM,&CPU::ADC,2 };
		lookup[0x65] = { "ADC",&CPU::ZPG,&CPU::ADC,3 };
		lookup[0x61] = { "ADC",&CPU::IND_X,&CPU::ADC,6 };
		lookup[0x6D] = { "ADC",&CPU::ABS,&CPU::ADC,4 };
		lookup[0xE9] = { "SBC",&CPU::IMM,&CPU::SBC,2 };
		lookup[0xE5] = { "SBC",&CPU::ZPG,&CPU::SBC,3 };
		lookup[0xE1] = { "SBC",&CPU::IND_X,&CPU::SBC,6 };
		lookup[0xED] = { "SBC",&CPU::ABS,&CPU::SBC,4 };
		lookup[0x46] = { "LSR",&CPU::ZPG,&CPU::LSR,5 };
		lookup[0x4E] = { "LSR",&CPU::ABS,&CPU::LSR,6 };
		lookup[0x4A] = { "LSR A",&CPU::IMP,&CPU::LSR_A,2 };
		lookup[0x06] = { "ASL",&CPU::ZPG,&CPU::ASL,5 };
		lookup[0x0E] = { "ASL",&CPU::ABS,&CPU::ASL,6 };
		lookup[0x0A] = { "ASL A",&CPU::IMP,&CPU::ASL_A,2 };
		lookup[0x66] = { "ROR",&CPU::ZPG,&CPU::ROR,5 };
		lookup[0x6E] = { "ROR",&CPU::ABS,&CPU::ROR,6 };
		lookup[0x6A] = { "ROR A",&CPU::IMP,&CPU::ROR_A,2 };
		lookup[0x26] = { "ROL",&CPU::ZPG,&CPU::ROL,5 };
		lookup[0x2E] = { "ROL",&CPU::ABS,&CPU::ROL,6 };
		lookup[0x2A] = { "ROL A",&CPU::IMP,&CPU::ROL_A,2 };
		lookup[0xC9] = { "CMP",&CPU::IMM,&CPU::CMP,2 };
		lookup[0xC5] = { "CMP",&CPU::ZPG,&CPU::CMP,3};
		lookup[0xC1] = { "CMP",&CPU::IND_X,&CPU::CMP,6 };
		lookup[0xCD] = { "CMP",&CPU::ABS,&CPU::CMP,4 };
		lookup[0xC0] = { "CPY",&CPU::IMM,&CPU::CPY,2 };
		lookup[0xC4] = { "CPY",&CPU::ZPG,&CPU::CPY,3 };
		lookup[0xCC] = { "CPY",&CPU::ABS,&CPU::CPY,4 };
		lookup[0xE0] = { "CPX",&CPU::IMM,&CPU::CPX,2 };
		lookup[0xE4] = { "CPX",&CPU::ZPG,&CPU::CPX,3 };
		lookup[0xEC] = { "CPX",&CPU::ABS,&CPU::CPX,4 };
		lookup[0x85] = { "STA",&CPU::ZPG,&CPU::STA,3 };
		lookup[0x8D] = { "STA",&CPU::ABS,&CPU::STA,4 };
		lookup[0x81] = { "STA",&CPU::IND_X,&CPU::STA,6 };
		lookup[0x86] = { "STX",&CPU::ZPG,&CPU::STX,3 };
		lookup[0x8E] = { "STX",&CPU::ABS,&CPU::STX,4 };
		lookup[0x84] = { "STY",&CPU::ZPG,&CPU::STY,3 };
		lookup[0x8C] = { "STY",&CPU::ABS,&CPU::STY,4 };
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
		lookup[0xA1] = { "LDA",&CPU::IND_X,&CPU::LDA,6 };
		lookup[0xF0] = { "BEQ",&CPU::REL,&CPU::BEQ,2 };
		lookup[0xD0] = { "BNE",&CPU::REL,&CPU::BNE,2 };
		lookup[0x24] = { "BIT",&CPU::ZPG,&CPU::BIT,3 };
		lookup[0x2C] = { "BIT",&CPU::ABS,&CPU::BIT,4 };
		lookup[0x09] = { "ORA",&CPU::IMM,&CPU::ORA,2 };
		lookup[0x05] = { "ORA",&CPU::ZPG,&CPU::ORA,3 };
		lookup[0x01] = { "ORA",&CPU::IND_X,&CPU::ORA,6 };
		lookup[0x0D] = { "ORA",&CPU::ABS,&CPU::ORA,4 };
		lookup[0xB8] = { "CLV",&CPU::IMP,&CPU::CLV,2 };
		lookup[0xC8] = { "INY",&CPU::IMP,&CPU::INY,2 };
		lookup[0xE8] = { "INX",&CPU::IMP,&CPU::INX,2 };
		lookup[0xE6] = { "INC",&CPU::ZPG,&CPU::INC,5 };
		lookup[0xEE] = { "INC",&CPU::ABS,&CPU::INC,6 };
		lookup[0x88] = { "DEY",&CPU::IMP,&CPU::DEY,2 };
		lookup[0xCA] = { "DEX",&CPU::IMP,&CPU::DEX,2 };
		lookup[0xC6] = { "DEC",&CPU::ZPG,&CPU::DEC,5 };
		lookup[0xCE] = { "DEC",&CPU::ABS,&CPU::DEC,6 };
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

	using DisasmEventHandler = void(*)(const DisasmEvent&);
	void registerDisasmEventHandler(DisasmEventHandler fn) { disasmEventHandler = fn; }

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

	DisasmEvent disasmEventData{};
	DisasmEventHandler disasmEventHandler{ nullptr };
	void populateDisasmEventData(u16 prevPC);

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
	int IND_X();

	/// execution
	// the actual logic
	int JMP();
	int LDX();
	int LDY();
	int STA();
	int STX();
	int STY();
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
	int LSR_A();
	int ASL();
	int ASL_A();
	int ROR();
	int ROR_A();
	int ROL();
	int ROL_A();
	int INY();
	int INX();
	int INC();
	int DEY();
	int DEX();
	int DEC();
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