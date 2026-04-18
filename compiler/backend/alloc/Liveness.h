#pragma once

#include "Allocation.h"

#include <unordered_set>

namespace backend::alloc {

struct LivenessInfo final {
    std::vector<std::unordered_set<std::size_t>> live_in;
    std::vector<std::unordered_set<std::size_t>> live_out;
};

LivenessInfo compute_liveness(const ir::FunctionIR& function);

} // namespace backend::alloc
