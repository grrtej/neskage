#include "CPU.h"

// dont include in CPU.h !!
#include "Bus.h"

u8 CPU::read(u16 addr) { return bus->read(addr); }
void CPU::write(u16 addr, u8 value) { bus->write(addr, value); }

void CPU::fetch() {
	opcode = read(pc++);
}

void CPU::tick() {
	if (cycles == 0) {
		fetch();

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

void CPU::ABS() {
	u8 lo = read(pc);
	u8 hi = read(pc + 1);

	addr_abs = (hi << 8) | lo;
}

void CPU::JMP() {
	pc = addr_abs;
}