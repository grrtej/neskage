#pragma once

#include "types.h"

#include <string_view>
#include <vector>

struct DisasmEvent {
	u8 a{};
	u8 x{};
	u8 y{};
	u8 status{};
	u8 sp{};
	u16 pc{};

	u64 totalCycles{};
	u16 addr{};
	u8 memVal{};

	std::string_view name{};
	std::vector<u8> ops{};

	// bleeds into DisasmEvent, so access using DisasmEvent::ABS etc
	// move this out to enum class when needed in somewhere else too
	// and probably create a map fn CPU::ABS funcptr -> AddrMode::ABS
	// ideally, it would be nice to just use addrModeFunc as a key
	//  (maybe i can? but that would pull CPU.h as a dep, worth it?)
	enum {
		ABS,
		IMM,
		ZPG,
		IMP,
		REL,
		IND_X,
	} addrMode{};
};