#include "IRPrint.h"

namespace ir {

std::string print(const FunctionIR& function) {
    return to_string(function);
}

std::string print(const ModuleIR& module) {
    return to_string(module);
}

std::string print(const BlockFunctionIR& function) {
    return to_string(function);
}

std::string print(const BlockModuleIR& module) {
    return to_string(module);
}

} // namespace ir
