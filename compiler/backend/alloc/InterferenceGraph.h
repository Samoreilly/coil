#pragma once

#include "Liveness.h"

#include <unordered_set>

namespace backend::alloc {

struct InterferenceGraph final {
    std::unordered_map<std::size_t, std::unordered_set<std::size_t>> edges;

    void add_node(std::size_t temp);
    void add_edge(std::size_t lhs, std::size_t rhs);
    std::unordered_set<std::size_t> neighbors(std::size_t temp) const;
    std::size_t degree(std::size_t temp) const;
};

InterferenceGraph build_interference_graph(const ir::FunctionIR& function, const LivenessInfo& liveness);

} // namespace backend::alloc
