#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

int main() {
    const fs::path build_dir = fs::path("test/.build");
    const fs::path binary = build_dir / "coil";
    const fs::path source = build_dir / "ir_cli_smoke.coil";
    const fs::path output = build_dir / "ir_cli_smoke.out";

    {
        std::ofstream out(source);
        out << "fn smoke() -> int { return 1; }\n";
    }

    const std::string command = binary.string() + " --ir " + source.string() + " > " + output.string();
    const int rc = std::system(command.c_str());
    if (rc != 0) {
        std::cerr << "compiler invocation failed\n";
        return 1;
    }

    std::ifstream in(output);
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const std::string expected = "function smoke\nentry_0\n  entry_0:\n  return 1\n";

    if (text != expected) {
        std::cerr << "unexpected printed IR\n";
        std::cerr << "Expected:\n" << expected;
        std::cerr << "Actual:\n" << text;
        return 1;
    }

    return 0;
}
