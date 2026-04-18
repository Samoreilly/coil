#pragma once

#include "TAC.h"

#include <string>
#include <vector>

namespace ir {

struct BasicBlock final {
    std::string name;
    std::vector<Quadruple> quads;
    std::vector<std::string> successors;

    bool terminated() const;
};

struct BlockFunctionIR final {
    std::string name;
    std::vector<BasicBlock> blocks;
};

struct BlockModuleIR final {
    std::vector<BlockFunctionIR> functions;
};

class BasicBlockBuilder final {
public:
    explicit BasicBlockBuilder(std::string function_name);

    BasicBlockBuilder(const BasicBlockBuilder&) = delete;
    BasicBlockBuilder& operator=(const BasicBlockBuilder&) = delete;

    BasicBlockBuilder(BasicBlockBuilder&&) noexcept = default;
    BasicBlockBuilder& operator=(BasicBlockBuilder&&) noexcept = default;

    BasicBlock& start_block(std::string name = {});
    Quadruple& emit(Opcode opcode, Operand result = {}, Operand arg1 = {}, Operand arg2 = {});
    Quadruple& emit_assign(Operand destination, Operand source);
    Quadruple& emit_binary(Opcode opcode, Operand destination, Operand lhs, Operand rhs);
    Quadruple& emit_unary(Opcode opcode, Operand destination, Operand value);
    Quadruple& emit_jump(Label target);
    Quadruple& emit_jump_if_true(Operand condition, Label target);
    Quadruple& emit_jump_if_false(Operand condition, Label target);
    Quadruple& emit_param(Operand value);
    Quadruple& emit_call(Operand destination, Operand callee, std::size_t argument_count);
    Quadruple& emit_return(Operand value = {});
    Quadruple& emit_label(Label label);

    BlockFunctionIR finish();

private:
    BlockFunctionIR function_;
    BasicBlock* current_block_ = nullptr;
    std::size_t next_block_id_ = 0;
    std::size_t next_temp_ = 0;
    std::size_t next_label_ = 0;

    BasicBlock& ensure_block();
    Label make_label(std::string_view hint = {});
    Temp make_temp();
};

std::string to_string(const BasicBlock& block);
std::string to_string(const BlockFunctionIR& function);
std::string to_string(const BlockModuleIR& module);

BlockFunctionIR blockify(const FunctionIR& function);
BlockModuleIR blockify(const ModuleIR& module);

} // namespace ir
