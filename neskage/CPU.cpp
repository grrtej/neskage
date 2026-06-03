#include "CPU.h"

#include "Bus.h"

void CPU::connectBus(Bus* b) {
	bus = b;
}

void CPU::tick() {
	if (cycles == 0) {
		opcode = read(pc++);

		switch (opcode) {
		case 0x4C: {
			cycles = 3;
			ABS();
			JMP();
		}break;
		}
	}

	cycles--;
}

u8 CPU::read(u16 addr) {
	return bus->read(addr);
}

void CPU::write(u16 addr, u8 value) {
	bus->write(addr, value);
}

void CPU::ABS() {
	u8 lo = read(pc++);
	u8 hi = read(pc++);
	addrAbs = (hi << 8) | lo;
}

void CPU::JMP() {
	pc = addrAbs;
}