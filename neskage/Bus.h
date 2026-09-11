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

	/// devices on the bus
	// TODO: should the bus construct these?
	// TODO: is Bus really a good name for this class, what about System/NES/Motherboard?
	CPU cpu{};
	std::array<u8, 64 * 1024> ram{};
	std::array<u8, 8 * 1024> ppuRam{};
};