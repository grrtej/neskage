#include "Bus.h"

u8 Bus::read(u16 addr) {
	return ram[addr];
}

void Bus::write(u16 addr, u8 value) {
	ram[addr] = value;
}

void Bus::tick() {
	cpu.tick();
}