#include "compiler/middle-end/ir/TAC.h"
#include "compiler/middle-end/ir/BasicBlock.h"
#include "compiler/middle-end/ir/IRPrint.h"

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
    using namespace ir;

    FunctionBuilder builder("printer");
    auto dst = builder.new_temp();
    builder.emit_assign(temp(dst.id), integer(1));
    auto branch = builder.new_label("branch");
    builder.emit_jump_if_false(boolean(true), branch);
    builder.emit_return(temp(dst.id));
    builder.emit_label(branch);
    builder.emit_return(temp(dst.id));
    auto function = builder.finish();

    auto tac_text = print(function);
    expect(tac_text.find("function printer") != std::string::npos, "tac printer function header");
    expect(tac_text.find("return t0") != std::string::npos, "tac printer return");

    auto blocks = blockify(ModuleIR{std::vector<FunctionIR>{function}});
    auto block_text = print(blocks);
    expect(block_text.find("function printer") != std::string::npos, "block printer function header");
    expect(block_text.find("entry") != std::string::npos, "block printer entry block");
    expect(block_text.find("successors") != std::string::npos, "block printer successors");

    if (failures != 0) {
        std::cerr << failures << " IR printer test(s) failed\n";
        return 1;
    }

    return 0;
}
