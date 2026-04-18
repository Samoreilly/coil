#include "compiler/backend/codegen/CodeGen.h"
#include "compiler/middle-end/ir/BasicBlock.h"

#include <iostream>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

} // namespace

int main() {
    using namespace backend::codegen;
    using namespace ir;

    BlockFunctionIR function;
    function.name = "add_one";
    function.blocks.push_back(BasicBlock{"entry", {}, {}});
    function.blocks.front().quads.push_back({Opcode::Assign, Temp{0}, IntegerLiteral{1}, {}});
    function.blocks.front().quads.push_back({Opcode::Add, Temp{1}, Temp{0}, IntegerLiteral{2}});
    function.blocks.front().quads.push_back({Opcode::Return, {}, Temp{1}, {}});

    Semantic::SymbolTable scope{"global"};
    auto asm_text = generate_assembly(function, scope);

    expect(asm_text.find(".globl add_one") != std::string::npos, "global symbol emitted");
    expect(asm_text.find("add_one:") != std::string::npos, "function label emitted");
    expect(asm_text.find("ret") != std::string::npos, "return emitted");

    if (failures != 0) {
        std::cerr << failures << " codegen test(s) failed\n";
        return 1;
    }

    return 0;
}
