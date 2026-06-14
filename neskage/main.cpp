#include <fstream>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include "types.h"
#include "Bus.h"

int load_nrom_128(Bus& nes, const char* path);

int main(int, char**)
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window* window{ SDL_CreateWindow("neskage", 800,800,0) };
	SDL_Renderer* renderer{ SDL_CreateRenderer(window,nullptr) };

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer3_Init(renderer);

	Bus nes{};
	int err = load_nrom_128(nes, "nestest.nes");
	if (err != 0) {
		return err;
	}

	bool done{ false };
	int cycleCount{ 0 };
	while (!done) {
		SDL_Event event{};
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);

			if (event.type == SDL_EVENT_QUIT) {
				done = true;
			}
		}

		if (cycleCount < 5) {
			nes.tick();
			cycleCount++;
		}

		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		ImGui::ShowDemoWindow();

		SDL_RenderClear(renderer);
		ImGui::Render();
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
		SDL_RenderPresent(renderer);
	}

	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
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