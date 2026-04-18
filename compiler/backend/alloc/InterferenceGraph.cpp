#include "InterferenceGraph.h"

namespace backend::alloc {

void InterferenceGraph::add_node(std::size_t temp) {
    edges.try_emplace(temp);
}

void InterferenceGraph::add_edge(std::size_t lhs, std::size_t rhs) {
    if (lhs == rhs) {
        return;
    }

    edges[lhs].insert(rhs);
    edges[rhs].insert(lhs);
}

std::unordered_set<std::size_t> InterferenceGraph::neighbors(std::size_t temp) const {
    if (auto it = edges.find(temp); it != edges.end()) {
        return it->second;
    }

    return {};
}

std::size_t InterferenceGraph::degree(std::size_t temp) const {
    if (auto it = edges.find(temp); it != edges.end()) {
        return it->second.size();
    }

    return 0;
}

InterferenceGraph build_interference_graph(const ir::FunctionIR& function, const LivenessInfo& liveness) {
    InterferenceGraph graph;

    for (std::size_t i = 0; i < function.quads.size(); ++i) {
        const auto& quad = function.quads[i];
        const auto live = i < liveness.live_out.size() ? liveness.live_out[i] : std::unordered_set<std::size_t>{};
        const auto defined = [&] {
            std::unordered_set<std::size_t> out;
            if (const auto* temp = std::get_if<ir::Temp>(&quad.result)) {
                out.insert(temp->id);
            }
            return out;
        }();

        for (const auto& temp : defined) {
            graph.add_node(temp);
            for (const auto& other : live) {
                graph.add_node(other);
                graph.add_edge(temp, other);
            }
        }

        auto collect = [&](const ir::Operand& operand) {
            if (const auto* temp = std::get_if<ir::Temp>(&operand)) {
                graph.add_node(temp->id);
            }
        };

        collect(quad.arg1);
        collect(quad.arg2);
    }

    return graph;
}

} // namespace backend::alloc
