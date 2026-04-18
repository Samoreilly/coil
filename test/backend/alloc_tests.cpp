#include "compiler/backend/alloc/Allocation.h"
#include "compiler/middle-end/ir/TAC.h"

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
    using namespace backend::alloc;
    using namespace ir;

    FunctionIR function;
    function.name = "alloc";
    // Build a tiny triangle in the interference graph so 2 registers are insufficient.
    function.quads.push_back({Opcode::Add, Temp{0}, Temp{1}, Temp{2}});
    function.quads.push_back({Opcode::Add, Temp{1}, Temp{0}, Temp{2}});
    function.quads.push_back({Opcode::Add, Temp{2}, Temp{0}, Temp{2}});
    function.quads.push_back({Opcode::Return, {}, Temp{0}, {}});

    ModuleIR module;
    module.functions.push_back(function);

    AllocatorConfig config;
    config.general_registers = {PhysicalRegister{"r0"}, PhysicalRegister{"r1"}};

    auto report = color_graph(module.functions.front(), config);

    expect(report.colored_count == 2, "colored count");
    expect(report.spill_count == 1, "spill count");
    expect(report.map.temps.at(0).physical.has_value(), "temp0 colored");
    expect(report.map.temps.at(1).physical.has_value(), "temp1 colored");
    expect(report.map.temps.at(2).spill.has_value(), "temp2 spilled");

    auto block_report = color_graph(ir::BlockFunctionIR{module.functions.front().name, {}}, config);
    expect(block_report.colored_count == 0, "empty block graph colored count");

    if (failures != 0) {
        std::cerr << failures << " allocator test(s) failed\n";
        return 1;
    }

    return 0;
}
