# AGENTS.md

Learning NES emulator (6502 interpreter), Windows/MSVC-only. No tests, no CI, no lint. Personal teaching project — don't over-engineer.

## Build

- Open `neskage.slnx` in Visual Studio 2022 (C++20, toolset v145, note: `.slnx` is the new XML solution format, not `.sln`) or:
  ```
  msbuild neskage.slnx /p:Configuration=Debug /p:Platform=x64
  ```
- Submodules are required (SDL3 `release-3.4.10`, imgui, ImGuiTextSelect):
  ```
  git submodule update --init --recursive
  ```
- App project is `neskage/neskage.vcxproj`. New UI/imgui code must be added to `external/imgui-build/imgui-build.vcxproj` (static lib that bakes imgui + SDL3 backends + textselect), which the app references. ImgUI is recent upstream (SDL3 backends, `#include <SDL3/SDL.h>`).

## Runtime gotchas

- The app loads `nestest.nes` from the CWD; it is gitignored (`*.nes`) and NOT in the repo. After a fresh clone, copy it into `neskage/` (it sits next to the sources) or startup fails. Running the exe from `x64\Debug` won't find it — VS Debug launches from `neskage/`.
- Console is disabled (`SubSystem=Windows`) even though `_CONSOLE` is defined; all output goes to the ImGui "Log" window.
- On start it auto-runs to cycle `6087` (`ON_START_RUN_UNTIL`, main.cpp); use "Step" / "Run Until Cycle" in the Controls window.

## Architecture

- `Bus` (Bus.h/cpp) owns `CPU`, 64K `ram`, 8K `ppuRam`. Read/write are flat piles into `ram` (no memory mapping yet). `tick()` is a single CPU tick.
- Instruction loop: call `nes->tick()` repeatedly until `nes->cpu.instructionDone()` is true (one instruction), then render.
- `CPU`: the opcode→`{name, addrMode, operate, cycles}` lookup table is populated inline in the `CPU::CPU()` constructor in **CPU.h**, not a data table. Adding an opcode means: lookup entry + `int CPU::xxx()` operate (and often an addressing-mode `int xxx()`), then a `cycles` number.
- `CPU` takes a raw `Bus*` via `connectBus`; lifetime is external. Disassembly is emitted per instruction through the `DisasmEventHandler` (see `event.h`) to `Logger` (Logger.cpp), which renders Nintendulator-style lines and is unbounded (Log buffer grows forever).

## Verification

- CPU correctness is checked by comparing the Log output to the canonical Nintendulator `nestest.log`. Init constants (pc=`0xC000`, sp=`0xFD`, totalCycles=`7`) are deliberately tuned to match that log — don't "fix" them casually.
- PPU today is only the pattern-table viewer (`populatePatternTablePixels`, main.cpp) reading `ppuRam`; current branch is `init-ppu`.

## Style quirks

- `types.h` aliases `u8/u16/u32/u64/i8`.
- Pattern-table texture MUST be `SDL_PIXELFORMAT_BGRA8888` with colors packed `(b<<24)|(g<<16)|(r<<8)|0xFF` — RGBA8888 mangles the channels (see main.cpp comment).
- Tabs for indentation, uses `<format>` (`std::format`), MSVC-only.