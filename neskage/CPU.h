#pragma once

#include "types.h"

// fwd decl Bus to use as a pointer type
class Bus;

class CPU
{
public:
	void tick();
	void connectBus(Bus* b) { bus = b; }

private:
	// communication bus
	// ps: this is an aggregate object relation
	// its lifetime is independent of CPU
	Bus* bus{ nullptr };

private:
	// 6502 state
	u16 pc{ 0xC000 };

private:
	// helpers
	int cycles{ 0 };

	u8 read(u16 addr);
	void write(u16 addr, u8 value);

	u8 opcode{};
	void fetch();

private:
	// addressing modes
	u16 addr_abs{};
	void ABS();

private:
	// execution
	void JMP();
};