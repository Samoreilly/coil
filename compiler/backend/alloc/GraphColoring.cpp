#include "GraphColoring.h"

#include <algorithm>

namespace backend::alloc {

bool AllocationChoice::is_spilled() const {
    return spill.has_value() && !physical.has_value();
}

namespace {

AllocationReport color_function(const ir::FunctionIR& function, const AllocatorConfig& config) {
    AllocationReport report;
    auto liveness = compute_liveness(function);
    auto graph = build_interference_graph(function, liveness);

    std::vector<std::size_t> nodes;
    nodes.reserve(graph.edges.size());
    for (const auto& [node, _] : graph.edges) {
        nodes.push_back(node);
    }

    std::sort(nodes.begin(), nodes.end(), [&](std::size_t lhs, std::size_t rhs) {
        const auto lhs_degree = graph.degree(lhs);
        const auto rhs_degree = graph.degree(rhs);
        if (lhs_degree != rhs_degree) {
            return lhs_degree < rhs_degree;
        }
        return lhs < rhs;
    });

    std::unordered_map<std::size_t, PhysicalRegister> assigned;
    std::size_t spill_index = 0;

    for (const auto& temp : nodes) {
        std::unordered_set<std::string> used_registers;
        for (const auto& neighbor : graph.neighbors(temp)) {
            if (auto it = assigned.find(neighbor); it != assigned.end()) {
                used_registers.insert(it->second.name);
            }
        }

        const PhysicalRegister* selected = nullptr;
        for (const auto& reg : config.general_registers) {
            if (!used_registers.contains(reg.name)) {
                selected = &reg;
                break;
            }
        }

        if (selected) {
            assigned.emplace(temp, *selected);
            ++report.colored_count;
            report.map.temps[temp] = AllocationChoice{*selected, std::nullopt};
        } else {
            SpillSlot slot{spill_index++};
            ++report.spill_count;
            report.map.temps[temp] = AllocationChoice{std::nullopt, slot};
        }
    }

    return report;
}

} // namespace

AllocationReport color_graph(const ir::FunctionIR& function, const AllocatorConfig& config) {
    return color_function(function, config);
}

AllocationReport color_graph(const ir::BlockFunctionIR& function, const AllocatorConfig& config) {
    ir::FunctionIR flattened;
    flattened.name = function.name;
    for (const auto& block : function.blocks) {
        flattened.quads.insert(flattened.quads.end(), block.quads.begin(), block.quads.end());
    }

    return color_function(flattened, config);
}

} // namespace backend::alloc
