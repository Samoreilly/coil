#pragma once

#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace backend::codegen {

struct AssemblyProgram final {
    std::vector<std::string> top_level_lines;
    std::vector<std::string> text_lines;
    std::vector<std::string> rodata_lines;

    void emit_top_level(std::string line) {
        top_level_lines.push_back(std::move(line));
    }

    void emit_text(std::string line) {
        text_lines.push_back(std::move(line));
    }

    void emit_rodata(std::string line) {
        rodata_lines.push_back(std::move(line));
    }

    std::string render() const {
        std::ostringstream out;
        for (const auto& line : top_level_lines) {
            out << line << '\n';
        }

        out << ".text\n";
        for (const auto& line : text_lines) {
            out << line << '\n';
        }

        if (!rodata_lines.empty()) {
            out << ".section .rodata\n";
            for (const auto& line : rodata_lines) {
                out << line << '\n';
            }
        }

        return out.str();
    }
};

} // namespace backend::codegen
