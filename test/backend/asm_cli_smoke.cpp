#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    const fs::path build_dir = fs::absolute(fs::path(argv[0])).parent_path();
    const fs::path binary = build_dir / "coil";
    const fs::path source = build_dir / "asm_cli_smoke.coil";
    const fs::path output = build_dir / "asm_cli_smoke.out";

    {
        std::ofstream out(source);
        out << "fn smoke() -> int { return 1; }\n";
    }

    const std::string command = binary.string() + " --asm " + source.string() + " > " + output.string();
    const int rc = std::system(command.c_str());
    if (rc != 0) {
        std::cerr << "compiler invocation failed\n";
        return 1;
    }

    std::ifstream in(output);
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    if (text.find("smoke:") == std::string::npos || text.find("ret") == std::string::npos) {
        std::cerr << "unexpected printed assembly\n";
        std::cerr << text;
        return 1;
    }

    return 0;
}
