#include "Logger.h"

#include <string>
#include <format>

#include <imgui.h>

// global scope, static duration, internal linkage
namespace {
	std::string line{};
	std::string partState{};
	std::string partDisasm{};
	std::string colPC{}, colOperands{}, colDisasm{};
	constexpr int colPCWidth{ 4 + 2 }, colOperandsWidth{ 8 + 2 }, colDisasmWidth{ 30 + 2 };

	struct {
		// or maybe just a simple vec<str> ? might be slower tho
		ImGuiTextBuffer buf{};
		std::vector<int> lineOffsets{};

		void addLine(const char* line) {
			lineOffsets.push_back(buf.size());
			buf.append(line);
		}

		std::string_view getLine(int idx) {
			const char* bufStart = buf.begin();
			const char* bufEnd = buf.end();
			int maxLines = getNumLines();

			const char* lineStart = bufStart + lineOffsets[idx];
			const char* lineEnd = (idx + 1 < maxLines) ? (bufStart + lineOffsets[idx + 1] - 1) : bufEnd;
			return { lineStart,lineEnd };
		}

		int getNumLines() {
			return static_cast<int>(lineOffsets.size());
		}

		void draw() {
			// TODO: select and copy
			if (ImGui::Begin("Log")) {
				for (int i = 0; i < getNumLines(); i++) {
					auto line = getLine(i);
					ImGui::TextUnformatted(line.data(), line.data() + line.size());
				}

				if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
					ImGui::SetScrollHereY(1.0f);
				}
			}
			ImGui::End();
		}
	} logWidget;
}

void Logger::onDisasmEvent(const DisasmEvent& e)
{
	partState = std::format("A:{:02X} X:{:02X} Y:{:02X} P:{:02X} SP:{:02X} CYC:{}", e.a, e.x, e.y, e.status, e.sp, e.totalCycles);

	colPC = std::format("{:04X}", e.pc);
	colOperands = std::format("{:02X}", e.ops[0]);
	colDisasm = e.name;

	if (e.addrMode == DisasmEvent::ABS) {
		colOperands += std::format(" {:02X} {:02X}", e.ops[1], e.ops[2]); // OP1 OP2
		colDisasm += std::format(" ${:04X}", e.addr);
		if (e.name != "JMP" && e.name != "JSR") {
			colDisasm += std::format(" = {:02X}", e.memVal);
		}
	}

	if (e.addrMode == DisasmEvent::IMM) {
		colOperands += std::format(" {:02X}", e.ops[1]); // OP1
		colDisasm += std::format(" #${:02X}", e.memVal);
	}

	if (e.addrMode == DisasmEvent::ZPG) {
		colOperands += std::format(" {:02X}", e.ops[1]); // OP1
		colDisasm += std::format(" ${:02X}", e.addr);
		colDisasm += std::format(" = {:02X}", e.memVal);
	}

	if (e.addrMode == DisasmEvent::REL) {
		colOperands += std::format(" {:02X}", e.ops[1]); // OP1
		colDisasm += std::format(" ${:04X}", e.addr);
	}

	if (e.addrMode == DisasmEvent::IND_X) {
		colOperands += std::format(" {:02X}", e.ops[1]); // OP1
		colDisasm += std::format(" (${:02X},X) @ {:02X} = {:04X} = {:02X}", e.ops[1], (e.ops[1] + e.x) % 256, e.addr, e.memVal);
	}

	partDisasm = std::format("{:{}}{:{}}{:{}}", colPC, colPCWidth, colOperands, colOperandsWidth, colDisasm, colDisasmWidth);

	line = partDisasm + partState + "\n";
	logWidget.addLine(line.c_str());
}

void Logger::draw() {
	logWidget.draw();
}