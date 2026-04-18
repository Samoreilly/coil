#pragma once

#include "../../middle-end/ir/BasicBlock.h"

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace backend::alloc {

enum class RegisterClass {
    General,
    Float,
};

struct PhysicalRegister final {
    std::string name;
    RegisterClass reg_class = RegisterClass::General;
};

struct SpillSlot final {
    std::size_t index = 0;
};

struct AllocationChoice final {
    std::optional<PhysicalRegister> physical;
    std::optional<SpillSlot> spill;

    bool is_spilled() const;
};

struct AllocationMap final {
    std::unordered_map<std::size_t, AllocationChoice> temps;
};

struct AllocatorConfig final {
    std::vector<PhysicalRegister> general_registers;
    std::vector<PhysicalRegister> float_registers;
    std::size_t spill_cost_bias = 0;
};

struct AllocationReport final {
    AllocationMap map;
    std::size_t spill_count = 0;
    std::size_t colored_count = 0;
};

AllocationReport color_graph(const ir::BlockFunctionIR& function, const AllocatorConfig& config);
AllocationReport color_graph(const ir::FunctionIR& function, const AllocatorConfig& config);

} // namespace backend::alloc
