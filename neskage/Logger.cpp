#include "Logger.h"

#include <string>
#include <format>

#include <imgui.h>
#include <textselect.hpp>

// global scope, static duration, internal linkage
namespace {
	std::string partState{};
	std::string partDisasm{};
	std::string colPC{}, colOperands{}, colDisasm{};
	constexpr int colPCWidth{ 4 + 2 }, colOperandsWidth{ 8 + 2 }, colDisasmWidth{ 30 + 2 };

	struct {
		// or maybe just a simple vec<str> ? might be slower tho
		ImGuiTextBuffer buf{};
		std::vector<int> lineOffsets{};
		TextSelect textSelect{
			[this](std::size_t idx) { return getLine(idx); },
			[this] { return getNumLines(); }
		};
		ImGuiListClipper clipper;

		// for future optimizations, investigate add operation. could be a bottleneck
		// using vec: usually O(1), but worst case (realloc) it becomes O(n)
		// ring buffer, etc?
		void addLine(const std::string& line) {
			lineOffsets.push_back(buf.size());
			buf.append(line.c_str());
		}

		std::string_view getLine(std::size_t idx) {
			const char* bufStart = buf.begin();
			const char* bufEnd = buf.end();
			auto maxLines = getNumLines();

			const char* lineStart = bufStart + lineOffsets[idx];
			const char* lineEnd = (idx + 1 < maxLines) ? (bufStart + lineOffsets[idx + 1]) : bufEnd;
			return { lineStart,lineEnd };
		}

		std::size_t getNumLines() {
			return lineOffsets.size();
		}

		void draw() {
			if (ImGui::Begin("Log")) {
				ImGui::BeginChild("text", {}, 0, ImGuiWindowFlags_NoMove);
				clipper.Begin(static_cast<int>(getNumLines()));
				while (clipper.Step()) {
					for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
						auto line = getLine(row);
						ImGui::TextUnformatted(line.data(), line.data() + line.size());
					}
				}

				textSelect.update();

				if (!textSelect.hasSelection() && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
					ImGui::SetScrollHereY(1.0f);
				}
				ImGui::EndChild();
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

	logWidget.addLine(partDisasm + partState);
}

void Logger::draw() {
	logWidget.draw();
}