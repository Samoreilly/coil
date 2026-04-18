#include "compiler/middle-end/ir/TAC.h"
#include "compiler/middle-end/ir/BasicBlock.h"
#include "compiler/middle-end/ir/IRGenerator.h"

#include <iostream>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

} // namespace

int main() {
    using namespace ir;

    expect(to_string(Opcode::Add) == "add", "opcode string for add");
    expect(to_string(integer(7)) == "7", "integer literal string");
    expect(to_string(named("x")) == "x", "named operand string");

    FunctionBuilder builder("sum");
    auto lhs = builder.new_temp();
    auto rhs = builder.new_temp();
    auto done = builder.new_label("done");

    builder.emit_binary(Opcode::Add, temp(lhs.id), named("a"), named("b"));
    builder.emit_jump_if_false(boolean(true), done);
    builder.emit_label(done);
    builder.emit_return(temp(rhs.id));

    auto function = builder.finish();

    expect(function.name == "sum", "function name preserved");
    expect(function.quads.size() == 4, "quad count");
    expect(function.quads[0].opcode == Opcode::Add, "first opcode add");
    expect(function.quads[0].is_terminator() == false, "add is not terminator");
    expect(function.quads[1].is_terminator() == true, "branch is terminator");
    expect(to_string(function.quads[0]) == "t0 = a + b", "binary quad string");
    expect(to_string(function.quads[1]) == "ifFalse true goto done_0", "branch quad string");
    expect(to_string(function.quads[2]) == "done_0:", "label quad string");
    expect(to_string(function.quads[3]) == "return t1", "return quad string");

    ModuleIR module;
    module.functions.push_back(std::move(function));
    expect(module.functions.size() == 1, "module contains function");
    expect(module.functions.front().name == "sum", "module function name");

    BasicBlockBuilder block_builder("sum_blocks");
    auto& entry = block_builder.start_block("entry");
    block_builder.emit_binary(Opcode::Add, temp(0), named("lhs"), named("rhs"));
    block_builder.emit_return(temp(0));

    auto blocks = block_builder.finish();
    expect(blocks.blocks.size() == 1, "single basic block created");
    expect(entry.terminated(), "entry block terminated");
    expect(to_string(blocks).find("entry") != std::string::npos, "block string contains name");

    if (failures != 0) {
        std::cerr << failures << " TAC test(s) failed\n";
        return 1;
    }

    return 0;
}
