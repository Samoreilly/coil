#pragma once

#include "BasicBlock.h"

namespace ir {

std::string print(const FunctionIR& function);
std::string print(const ModuleIR& module);
std::string print(const BlockFunctionIR& function);
std::string print(const BlockModuleIR& module);

} // namespace ir
