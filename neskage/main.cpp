#include <fstream>
#include <vector>

#include "types.h"
#include "Bus.h"

int load_nrom_128(Bus& nes, const char* path);

int main()
{
	Bus nes{};

	int err = load_nrom_128(nes, "nestest.nes");
	if (err != 0) {
		return err;
	}

	for (int i = 0; i < 20; i++) {
		nes.tick();
	}

	return 0;
}

int load_nrom_128(Bus& nes, const char* path) {
	/// read the file into a vec u8
	// hope it doesnt mangle the data, idk enough c++ to be sure
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file) {
		return 1; // couldnt open file
	}
	auto size = file.tellg();
	std::vector<u8> cart(size);
	file.seekg(0);
	// reinterpret_cast allows using u8* where char* is expected
	// doesnt touch the bits i hope
	file.read(reinterpret_cast<char*>(cart.data()), size);

	/// basic ines signature confirmation
	if (!(cart[0] == 'N' && cart[1] == 'E' && cart[2] == 'S' && cart[3] == 0x1A)) {
		return 2; // not an ines file
	}

	/// copy prg rom into cpu ram where nestest expects, couldbe wrong
	// pretty sure there are faster ways to copy
	for (int i = 0; i < 16 * 1024; i++) {
		nes.write(0xc000 + i, cart[16 + i]);
	}

	return 0;
}