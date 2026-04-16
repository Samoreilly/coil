#include "../compiler/frontend/lexer/Lexer.h"
#include "../compiler/frontend/parser/Parser.h"
#include "../compiler/ast/visitors/PrintVisitor.h"
#include "../compiler/middle-end/semantic/RegisterVisitor.h"
#include "../compiler/middle-end/semantic/TypeCheckingVisitor.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>

namespace fs = std::filesystem;

enum class Stage {
    Lexer,
    Parser,
    Semantic,
    Integration
};

struct StderrSilencer {
    int saved_fd = -1;

    StderrSilencer() {
        std::fflush(stderr);
        saved_fd = ::dup(::fileno(stderr));
        int devnull = ::open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            ::dup2(devnull, ::fileno(stderr));
            ::close(devnull);
        }
    }

    ~StderrSilencer() {
        std::fflush(stderr);
        if (saved_fd >= 0) {
            ::dup2(saved_fd, ::fileno(stderr));
            ::close(saved_fd);
        }
    }
};

static std::string read_text(const fs::path& path) {
    std::ifstream in(path);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

static void write_text(const fs::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

static std::string read_expectation(const fs::path& path) {
    return read_text(path);
}
static std::string read_file(const fs::path& path) {
    return read_text(path);
}

static std::string with_file_marker(const fs::path& file, const fs::path& root) {
    auto rel = fs::relative(file, root).generic_string();
    return "[[FILE:" + rel + "]]" + "\n" + read_file(file);
}

static std::string dump_tokens(const std::vector<Token>& tokens) {
    std::ostringstream out;
    for (const auto& t : tokens) {
        out << tokenTypeToString.at(t.token_type) << ' ' << t.token_value << '\n';
    }
    return out.str();
}

static std::string dump_ast(Node* node) {
    std::ostringstream out;
    PrintVisitor printer(out);
    if (node) {
        node->accept(printer);
    }
    return out.str();
}

static bool contains_all(const std::string& haystack, const std::vector<std::string>& needles) {
    return std::all_of(needles.begin(), needles.end(), [&](const std::string& needle) {
        return haystack.find(needle) != std::string::npos;
    });
}

static bool contains_none(const std::string& haystack, const std::vector<std::string>& needles) {
    return std::all_of(needles.begin(), needles.end(), [&](const std::string& needle) {
        return haystack.find(needle) == std::string::npos;
    });
}

static Stage stage_for_path(const fs::path& path) {
    const auto folder = path.parent_path().filename().string();
    if (folder == "lexer") return Stage::Lexer;
    if (folder == "parser") return Stage::Parser;
    if (folder == "semantic") return Stage::Semantic;
    return Stage::Integration;
}

static std::string run_case(const fs::path& file, const fs::path& root) {
    Diagnostics diagnostics;
    const auto source = with_file_marker(file, root);
    StderrSilencer silence;

    Lexer lexer(diagnostics);
    lexer.lex(source);

    std::string output;
    const auto stage = stage_for_path(file);

    if (stage == Stage::Lexer) {
        output = diagnostics.has_errors() ? diagnostics.render() : dump_tokens(lexer.tokens);
    } else {
        Parser parser(std::move(lexer.tokens), diagnostics);
        auto node = parser.construct_ast();

        if (diagnostics.has_errors() || !node) {
            output = diagnostics.render();
        } else if (stage == Stage::Parser) {
            output = dump_ast(node.get());
        } else {
            Semantic::SymbolTable global_scope("global");
            RegisterVisitor register_sem{&global_scope, diagnostics};
            TypeCheckingVisitor type_checker{&global_scope, diagnostics};

            node->accept(register_sem);
            node->accept(type_checker);

            output = diagnostics.has_errors() ? diagnostics.render() : "";
        }
    }

    return output;
}

int main(int argc, char* argv[]) {
    bool refresh = false;
    int root_index = 1;

    if (argc >= 2 && std::string(argv[1]) == "--refresh") {
        refresh = true;
        root_index = 2;
    }

    if (argc <= root_index) {
        std::cerr << "usage: test_runner [--refresh] <test-root>\n";
        return 1;
    }

    const fs::path root = fs::path(argv[root_index]);
    int passed = 0;
    int failed = 0;

    const std::vector<fs::path> dirs = {
        root / "lexer",
        root / "parser",
        root / "semantic",
        root / "integration"
    };

    for (const auto& dir : dirs) {
        if (!fs::exists(dir)) {
            continue;
        }

        std::vector<fs::directory_entry> files;
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".coil") {
                continue;
            }
            files.push_back(entry);
        }

        std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) {
            return a.path().filename().string() < b.path().filename().string();
        });

        for (const auto& entry : files) {
            std::cout << "[RUN] " << fs::relative(entry.path(), root).generic_string() << std::endl;
            const auto expect_path = entry.path().parent_path() / (entry.path().stem().string() + ".expect");
            const auto output = run_case(entry.path(), root);

            if (refresh) {
                write_text(expect_path, output);
                std::cout << "[WRITE] " << fs::relative(expect_path, root).generic_string() << '\n';
                ++passed;
                continue;
            }

            if (!fs::exists(expect_path)) {
                std::cout << "[FAIL] " << fs::relative(entry.path(), root).generic_string() << " missing expectation file\n";
                ++failed;
                continue;
            }

            const auto expected = read_expectation(expect_path);
            if (output == expected) {
                std::cout << "[PASS] " << fs::relative(entry.path(), root).generic_string() << '\n';
                ++passed;
            } else {
                std::cout << "[FAIL] " << fs::relative(entry.path(), root).generic_string() << '\n';
                std::cout << "Expected:\n" << expected << '\n';
                std::cout << "Actual:\n" << output << '\n';
                ++failed;
            }
        }
    }

    std::cout << "Passed: " << passed << '\n';
    std::cout << "Failed: " << failed << '\n';
    return failed == 0 ? 0 : 1;
}
