#pragma once

#include "InterferenceGraph.h"

namespace backend::alloc {

AllocationReport color_graph(const ir::FunctionIR& function, const AllocatorConfig& config);
AllocationReport color_graph(const ir::BlockFunctionIR& function, const AllocatorConfig& config);

} // namespace backend::alloc
