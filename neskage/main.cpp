#include <fstream>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include "types.h"
#include "Bus.h"
#include "Logger.h"

int loadNROM128(Bus* nes, const char* path);

static constexpr u64 ON_START_RUN_UNTIL = 2330;

int main(int, char**)
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window{ SDL_CreateWindow("neskage", 1200,800,SDL_WINDOW_RESIZABLE) };
	SDL_Renderer* renderer{ SDL_CreateRenderer(window,nullptr) };

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer3_Init(renderer);

	Bus* nes{ new Bus };
	int err = loadNROM128(nes, "nestest.nes");
	if (err != 0) {
		return err;
	}
	nes->cpu.registerDisasmEventHandler(Logger::onDisasmEvent); // nintendulator like output (for nestest.log)

	u64 inputCycleCount = nes->cpu.getTotalCycleCount();
	const u64 inputCycleCountStep = 1;
	const u64 inputCycleCountStepFast = 10;
	u64 untilCycle{ ON_START_RUN_UNTIL };
	bool step{ false };

	SDL_Event event{};

	bool done{ false };
	while (!done) {
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);

			if (event.type == SDL_EVENT_QUIT) {
				done = true;
			}
		}

		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		/// update

		// buttons and inputs to control execution
		if (ImGui::Begin("Controls")) {
			if (ImGui::Button("Step")) { step = true; }
			ImGui::InputScalar("##", ImGuiDataType_U64, &inputCycleCount, &inputCycleCountStep, &inputCycleCountStepFast);
			ImGui::SameLine();
			if (ImGui::Button("Run Until Cycle")) { untilCycle = inputCycleCount; }
		}
		ImGui::End();

		if (step || (nes->cpu.getTotalCycleCount() < untilCycle)) {
			do {
				nes->tick();
			} while (!nes->cpu.instructionDone());

			inputCycleCount = nes->cpu.getTotalCycleCount();
			step = false;
		}

		ImGui::ShowDemoWindow();
		SDL_RenderClear(renderer);
		ImGui::Render();
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
		SDL_RenderPresent(renderer);
	}

	// deallocate the heap allocated Bus
	delete nes;
	nes = nullptr;

	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}

int loadNROM128(Bus* nes, const char* path) {
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
		nes->write(0xc000 + i, cart[16 + i]);
	}

	return 0;
}
