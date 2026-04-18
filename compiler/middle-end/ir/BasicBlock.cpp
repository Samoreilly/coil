#include "BasicBlock.h"

#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace ir {

bool BasicBlock::terminated() const {
    return !quads.empty() && is_terminator(quads.back());
}

BasicBlockBuilder::BasicBlockBuilder(std::string function_name) {
    function_.name = std::move(function_name);
}

BasicBlock& BasicBlockBuilder::start_block(std::string name) {
    if (name.empty()) {
        name = "B" + std::to_string(next_block_id_++);
    }

    function_.blocks.push_back(BasicBlock{std::move(name), {}, {}});
    current_block_ = &function_.blocks.back();
    return *current_block_;
}

BasicBlock& BasicBlockBuilder::ensure_block() {
    if (!current_block_) {
        return start_block();
    }

    return *current_block_;
}

Temp BasicBlockBuilder::make_temp() {
    return Temp{next_temp_++};
}

Label BasicBlockBuilder::make_label(std::string_view hint) {
    const auto id = next_label_++;
    std::string name = hint.empty() ? "L" + std::to_string(id) : std::string(hint) + "_" + std::to_string(id);
    return Label{id, std::move(name)};
}

Quadruple& BasicBlockBuilder::emit(Opcode opcode, Operand result, Operand arg1, Operand arg2) {
    auto& block = ensure_block();
    block.quads.push_back(Quadruple{opcode, std::move(result), std::move(arg1), std::move(arg2)});
    return block.quads.back();
}

Quadruple& BasicBlockBuilder::emit_assign(Operand destination, Operand source) {
    return emit(Opcode::Assign, std::move(destination), std::move(source));
}

Quadruple& BasicBlockBuilder::emit_binary(Opcode opcode, Operand destination, Operand lhs, Operand rhs) {
    return emit(opcode, std::move(destination), std::move(lhs), std::move(rhs));
}

Quadruple& BasicBlockBuilder::emit_unary(Opcode opcode, Operand destination, Operand value) {
    return emit(opcode, std::move(destination), std::move(value));
}

Quadruple& BasicBlockBuilder::emit_jump(Label target) {
    auto& quad = emit(Opcode::Jump, std::move(target));
    function_.blocks.back().successors.push_back(to_string(function_.blocks.back().quads.back().result));
    return quad;
}

Quadruple& BasicBlockBuilder::emit_jump_if_true(Operand condition, Label target) {
    auto& quad = emit(Opcode::JumpIfTrue, std::move(target), std::move(condition));
    function_.blocks.back().successors.push_back(to_string(function_.blocks.back().quads.back().result));
    return quad;
}

Quadruple& BasicBlockBuilder::emit_jump_if_false(Operand condition, Label target) {
    auto& quad = emit(Opcode::JumpIfFalse, std::move(target), std::move(condition));
    function_.blocks.back().successors.push_back(to_string(function_.blocks.back().quads.back().result));
    return quad;
}

Quadruple& BasicBlockBuilder::emit_param(Operand value) {
    return emit(Opcode::Param, {}, std::move(value));
}

Quadruple& BasicBlockBuilder::emit_call(Operand destination, Operand callee, std::size_t argument_count) {
    return emit(Opcode::Call, std::move(destination), std::move(callee), integer(static_cast<std::int64_t>(argument_count)));
}

Quadruple& BasicBlockBuilder::emit_return(Operand value) {
    auto& quad = emit(Opcode::Return, {}, std::move(value));
    return quad;
}

Quadruple& BasicBlockBuilder::emit_label(Label label) {
    return emit(Opcode::Label, std::move(label));
}

BlockFunctionIR BasicBlockBuilder::finish() {
    return std::move(function_);
}

std::string to_string(const BasicBlock& block) {
    std::ostringstream out;
    out << block.name << "\n";
    if (!block.successors.empty()) {
        out << "  successors: ";
        for (std::size_t i = 0; i < block.successors.size(); ++i) {
            if (i > 0) {
                out << ", ";
            }
            out << block.successors[i];
        }
        out << '\n';
    }
    for (const auto& quad : block.quads) {
        out << "  " << to_string(quad) << '\n';
    }
    return out.str();
}

std::string to_string(const BlockFunctionIR& function) {
    std::ostringstream out;
    out << "function " << function.name << "\n";
    for (const auto& block : function.blocks) {
        out << to_string(block);
    }
    return out.str();
}

std::string to_string(const BlockModuleIR& module) {
    std::ostringstream out;
    for (const auto& function : module.functions) {
        out << to_string(function);
    }
    return out.str();
}

BlockFunctionIR blockify(const FunctionIR& function) {
    BlockFunctionIR out;
    out.name = function.name;

    std::vector<BasicBlock> blocks;
    BasicBlock current;
    current.name = "entry";

    for (const auto& quad : function.quads) {
        if (quad.opcode == Opcode::Label) {
            if (!current.quads.empty()) {
                blocks.push_back(std::move(current));
                current = BasicBlock{};
            }

            current.name = label_name(std::get<Label>(quad.result));
        }

        current.quads.push_back(quad);

        if (quad.is_terminator()) {
            blocks.push_back(std::move(current));
            current = BasicBlock{};
        }
    }

    if (!current.quads.empty()) {
        blocks.push_back(std::move(current));
    }

    std::unordered_map<std::string, std::size_t> label_to_block;
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        if (!blocks[i].quads.empty() && blocks[i].quads.front().opcode == Opcode::Label) {
            label_to_block[label_name(std::get<Label>(blocks[i].quads.front().result))] = i;
        }
    }

    auto add_successor = [](BasicBlock& block, const std::string& name) {
        if (name.empty()) {
            return;
        }

        if (std::find(block.successors.begin(), block.successors.end(), name) == block.successors.end()) {
            block.successors.push_back(name);
        }
    };

    for (std::size_t i = 0; i < blocks.size(); ++i) {
        auto& block = blocks[i];
        const auto fallthrough = (i + 1 < blocks.size()) ? blocks[i + 1].name : std::string{};

        if (block.quads.empty()) {
            continue;
        }

        const auto& last = block.quads.back();
        auto resolve_label = [&](const Operand& operand) {
            if (const auto* lbl = std::get_if<Label>(&operand)) {
                const auto key = label_name(*lbl);
                const auto it = label_to_block.find(key);
                if (it != label_to_block.end()) {
                    add_successor(blocks[i], blocks[it->second].name);
                }
            }
        };

        switch (last.opcode) {
            case Opcode::Jump:
                resolve_label(last.result);
                break;
            case Opcode::JumpIfTrue:
            case Opcode::JumpIfFalse:
                resolve_label(last.result);
                add_successor(blocks[i], fallthrough);
                break;
            case Opcode::Return:
                break;
            default:
                add_successor(blocks[i], fallthrough);
                break;
        }
    }

    out.blocks = std::move(blocks);

    return out;
}

BlockModuleIR blockify(const ModuleIR& module) {
    BlockModuleIR out;
    out.functions.reserve(module.functions.size());

    for (const auto& function : module.functions) {
        out.functions.push_back(blockify(function));
    }

    return out;
}

} // namespace ir
