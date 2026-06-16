#pragma once

#include "CPU.h"
#include "types.h"
#include <array>

class Bus
{
public:
	Bus();
	u8 read(u16 addr);
	void write(u16 addr, u8 value);
	void tick();
	
	// is it worth leaking it like this ?
	CPU& getCPU() { return cpu; }

private:
	/// devices on the bus
	// TODO: should the bus construct these?
	// TODO: is Bus really a good name for this class, what about System/NES/Motherboard?
	CPU cpu{};
	std::array<u8, 64 * 1024> ram{};
};