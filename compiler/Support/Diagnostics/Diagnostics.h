#pragma once

#include <sstream>
#include <string>
#include <utility>
#include <vector>

enum class DiagPhase {
    LEXER,
    PARSER,
    SEMANTIC
};

inline const char* diag_phase_to_string(DiagPhase phase) {
    switch (phase) {
        case DiagPhase::LEXER: return "lexer";
        case DiagPhase::PARSER: return "parser";
        case DiagPhase::SEMANTIC: return "semantic";
    }

    return "unknown";
}

struct DiagInfo {
    DiagPhase phase = DiagPhase::PARSER;
    std::string message;
    int line = 0;
    int col = 0;
    std::string near;
    std::string file_name;

    DiagInfo() = default;

    DiagInfo(
        DiagPhase p,
        std::string m,
        int l = 0,
        int c = 0,
        std::string n = "",
        std::string f = "")
        : phase(p), message(std::move(m)), line(l), col(c), near(std::move(n)), file_name(std::move(f)) {}
};

class Diagnostics {
    std::vector<DiagInfo> errors;

public:

    Diagnostics() = default;

    void send(DiagInfo info) {
        errors.push_back(std::move(info));
    }

    void send(
        DiagPhase phase,
        std::string message,
        int line = 0,
        int col = 0,
        std::string near = "",
        std::string file_name = "") {
        errors.emplace_back(
            phase,
            std::move(message),
            line,
            col,
            std::move(near),
            std::move(file_name));
    }

    bool has_errors() const {
        return !errors.empty();
    }

    const std::vector<DiagInfo>& all() const {
        return errors;
    }

    void clear() {
        errors.clear();
    }

    std::string render() const {
        std::ostringstream out;

        out << "Found " << errors.size() << " error(s):\n";

        for (const auto& error : errors) {
            out << "[" << diag_phase_to_string(error.phase) << "] ";

            if (!error.file_name.empty()) {
                out << error.file_name;
                if (error.line > 0) {
                    out << ":";
                } else {
                    out << ": ";
                }
            }

            if (error.line > 0) {
                out << "line " << error.line;
                if (error.col > 0) {
                    out << ", col " << error.col;
                }
                out << ": ";
            }

            out << error.message;

            if (!error.near.empty()) {
                out << " (near '" << error.near << "')";
            }

            out << '\n';
        }

        return out.str();
    }
};
