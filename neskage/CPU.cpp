#include "CPU.h"

#include "Bus.h"

void CPU::connectBus(Bus* b) {
	bus = b;
}

void CPU::tick() {
	if (cycles == 0) {
		opcode = read(pc++);

		cycles = lookup[opcode].cycles;
		// kinda ugly syntax, but binds "this" to the member func call, then calls it
		// there's also std::invoke(lookup[opcode].addrMode, this)
		(this->*lookup[opcode].addrMode)();
		(this->*lookup[opcode].operate)();
	}

	cycles--;
}

u8 CPU::read(u16 addr) {
	return bus->read(addr);
}

void CPU::write(u16 addr, u8 value) {
	bus->write(addr, value);
}

int CPU::ABS() {
	u8 lo = read(pc++);
	u8 hi = read(pc++);
	addrAbs = (hi << 8) | lo;
	return 0;
}

int CPU::JMP() {
	pc = addrAbs;
	return 0;
}