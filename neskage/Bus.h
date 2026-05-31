#pragma once

#include "types.h"
#include "CPU.h"

#include <vector>

class Bus
{
public:
	Bus() { cpu.connectBus(this); }
	void tick();
	u8 read(u16 addr);
	void write(u16 addr, u8 value);

private:
	// devices on the bus
	// TODO: should the bus construct these?
	// TODO: is Bus really a good name for this class, what about System/NES/Motherboard?
	CPU cpu{};
	std::vector<u8> ram{ std::vector<u8>(64 * 1024,0) };
};