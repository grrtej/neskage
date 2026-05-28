#include <fstream>
#include <iostream>
#include <cstdint>
#include <vector>
#include <string>

using u8 = uint8_t;
using u16 = uint16_t;

int main()
{
	/// LOOKUP
	std::vector<std::string> instruction_names(256,"???");
	instruction_names[0x4C] = "JMP";

	/// CPU STATE
	std::vector<u8> cpu_ram(64 * 1024);
	u16 pc = 0xc000;

	/// read the file into a vec u8
	// hope it doesnt mangle the data, idk enough c++ to be sure
	std::ifstream nestest_file("nestest.nes", std::ios::binary | std::ios::ate);
	if (!nestest_file) {
		return 1; // couldnt open file
	}
	auto size = nestest_file.tellg();
	std::vector<u8> nestest_cart(size);
	nestest_file.seekg(0);
	// reinterpret_cast allows using u8* where char* is expected
	// doesnt touch the bits i hope
	nestest_file.read(reinterpret_cast<char*>(nestest_cart.data()), size);

	/// confirm ines signature
	if (!(nestest_cart[0] == 'N' && nestest_cart[1] == 'E' && nestest_cart[2] == 'S' && nestest_cart[3] == 0x1A)) {
		return 2; // not an ines file
	}

	/// copy prg rom into cpu ram where nestest expects, couldbe wrong
	for (int i = 0; i < 16 * 1024; i++) {
		cpu_ram[0xc000 + i] = nestest_cart[16+ i];
	}

	/// FETCH-DECODE-EXECUTE
	u8 opcode = cpu_ram[pc];
	pc++;
	switch (opcode) {
	case 0x4C: { // JMP
		pc = cpu_ram[pc];
	}break;
	}

	return 0;
}