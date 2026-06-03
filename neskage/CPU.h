#pragma once

#include "types.h"

// TODO: revisit this someday, understand the approach more deeply
// this might help: https://stackoverflow.com/a/628079
// 
// this confuses me -- the circular include stuff
// fwd decl, for use as ptr member
class Bus;

class CPU
{
public:
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
	void ABS();

	/// execution
	// the actual logic
	void JMP();
};