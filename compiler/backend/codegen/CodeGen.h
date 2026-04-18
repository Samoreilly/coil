#pragma once

#include "../alloc/Allocation.h"
#include "../../middle-end/ir/BasicBlock.h"
#include "../../middle-end/semantic/Semantic.h"

#include <string>

namespace backend::codegen {

struct CodegenConfig final {
    bool emit_comments = false;
    backend::alloc::AllocatorConfig allocator;
};

std::string generate_assembly(const ir::BlockFunctionIR& function, Semantic::SymbolTable& global_scope, const CodegenConfig& config = {});
std::string generate_assembly(const ir::BlockModuleIR& module, Semantic::SymbolTable& global_scope, const CodegenConfig& config = {});

std::string generate_assembly(const ir::BlockFunctionIR& function, const CodegenConfig& config = {});
std::string generate_assembly(const ir::BlockModuleIR& module, const CodegenConfig& config = {});

} // namespace backend::codegen
