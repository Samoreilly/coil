#include "CodeGen.h"

#include "Assembly.h"
#include "TypeLayout.h"

#include <algorithm>
#include <sstream>
#include <unordered_map>

namespace backend::codegen {

namespace {

using ir::BlockFunctionIR;
using ir::BlockModuleIR;
using ir::BasicBlock;
using ir::Opcode;
using ir::Operand;
using ir::Quadruple;
using ir::Temp;

struct FunctionState final {
    std::unordered_map<std::size_t, std::size_t> temp_slots;
    std::unordered_map<std::string, std::size_t> named_slots;
    std::unordered_map<std::string, std::string> block_labels;
    std::unordered_map<std::size_t, ValueLayout> temp_layouts;
    std::size_t next_slot = 0;
    std::size_t stack_bytes = 0;
};

std::string local_label(const std::string& function_name, const std::string& block_name) {
    return ".L" + function_name + "_" + block_name;
}

std::string slot_address(std::size_t slot) {
    return "-" + std::to_string((slot + 1) * 8) + "(%rbp)";
}

std::optional<std::size_t> variable_offset_for_name(const std::string& name, const Semantic::SymbolTable* scope) {
    if (!scope) {
        return std::nullopt;
    }

    if (const auto* entry = scope->lookup_global(name); entry && entry->node_kind == NodeKind::VARIABLE_NODE) {
        return static_cast<std::size_t>(entry->offset);
    }

    return std::nullopt;
}

std::string width_reg(const std::string& reg, std::size_t bits) {
    if (reg == "%r10") {
        if (bits <= 8) return "%r10b";
        if (bits <= 16) return "%r10w";
        if (bits <= 32) return "%r10d";
        return "%r10";
    }

    if (reg == "%r11") {
        if (bits <= 8) return "%r11b";
        if (bits <= 16) return "%r11w";
        if (bits <= 32) return "%r11d";
        return "%r11";
    }

    if (reg == "%rax") {
        if (bits <= 8) return "%al";
        if (bits <= 16) return "%ax";
        if (bits <= 32) return "%eax";
        return "%rax";
    }

    if (reg == "%rdx") {
        if (bits <= 8) return "%dl";
        if (bits <= 16) return "%dx";
        if (bits <= 32) return "%edx";
        return "%rdx";
    }

    return reg;
}

std::string scratch(std::size_t index) {
    return index == 0 ? "%r10" : "%r11";
}

ValueLayout layout_for_operand(const Operand& op, const FunctionState& state, const Semantic::SymbolTable* scope) {
    if (std::holds_alternative<ir::FloatingLiteral>(op)) return layout_for_type(TYPES.at("f64"));
    if (std::holds_alternative<ir::BooleanLiteral>(op)) return layout_for_type(TYPES.at("bool"));
    if (std::holds_alternative<ir::CharacterLiteral>(op)) return layout_for_type(TYPES.at("char"));
    if (std::holds_alternative<ir::StringLiteral>(op)) return layout_for_type(TYPES.at("string"));
    if (std::holds_alternative<ir::IntegerLiteral>(op)) return layout_for_type(TYPES.at("int"));

    if (const auto* named = std::get_if<ir::NamedValue>(&op)) {
        if (scope) {
            if (const auto* entry = scope->lookup_global(named->name); entry && entry->type) {
                return layout_for_type(*entry->type);
            }
        }
        return layout_for_type(TYPES.at("int"));
    }

    if (const auto* temp = std::get_if<Temp>(&op)) {
        if (auto it = state.temp_layouts.find(temp->id); it != state.temp_layouts.end()) {
            return it->second;
        }
    }

    return layout_for_type(TYPES.at("int"));
}

std::string operand_text(const Operand& op, FunctionState& state, const Semantic::SymbolTable* scope, std::ostringstream& out, const std::string& reg = "%r10") {
    if (const auto* temp = std::get_if<Temp>(&op)) {
        return slot_address(state.temp_slots[temp->id]);
    }

    if (const auto* named = std::get_if<ir::NamedValue>(&op)) {
        if (auto offset = variable_offset_for_name(named->name, scope)) {
            return slot_address(*offset);
        }

        if (!state.named_slots.contains(named->name)) {
            state.named_slots[named->name] = state.next_slot++;
            state.stack_bytes = std::max(state.stack_bytes, state.next_slot * 8);
        }

        return slot_address(state.named_slots[named->name]);
    }

    if (const auto* integer = std::get_if<ir::IntegerLiteral>(&op)) {
        return "$" + std::to_string(integer->value);
    }

    if (const auto* boolean = std::get_if<ir::BooleanLiteral>(&op)) {
        return "$" + std::to_string(boolean->value ? 1 : 0);
    }

    if (const auto* chr = std::get_if<ir::CharacterLiteral>(&op)) {
        return "$" + std::to_string(static_cast<int>(chr->value));
    }

    if (const auto* floating = std::get_if<ir::FloatingLiteral>(&op)) {
        out << "  # floating literal " << floating->value << '\n';
        return reg;
    }

    if (std::holds_alternative<std::monostate>(op)) {
        return "$0";
    }

    return reg;
}

std::string label_for(const Operand& op, const FunctionState& state, const std::string& function_name) {
    if (const auto* label = std::get_if<ir::Label>(&op)) {
        if (auto it = state.block_labels.find(label->name); it != state.block_labels.end()) {
            return it->second;
        }
        return local_label(function_name, label->name);
    }
    return ir::to_string(op);
}

std::string compare_suffix(Opcode opcode) {
    switch (opcode) {
        case Opcode::CompareEq: return "e";
        case Opcode::CompareNe: return "ne";
        case Opcode::CompareLt: return "l";
        case Opcode::CompareLe: return "le";
        case Opcode::CompareGt: return "g";
        case Opcode::CompareGe: return "ge";
        default: return "e";
    }
}

void ensure_slot(FunctionState& state, std::size_t temp_id) {
    if (!state.temp_slots.contains(temp_id)) {
        state.temp_slots[temp_id] = state.next_slot++;
    }
    state.stack_bytes = std::max(state.stack_bytes, state.next_slot * 8);
}

void ensure_slot(FunctionState& state, const std::string& name) {
    if (!state.named_slots.contains(name)) {
        state.named_slots[name] = state.next_slot++;
    }
    state.stack_bytes = std::max(state.stack_bytes, state.next_slot * 8);
}

void reserve_scope_storage(FunctionState& state, const Semantic::SymbolTable* scope) {
    if (!scope) {
        return;
    }

    std::size_t max_slot = 0;
    bool found = false;
    for (const auto& [name, entry] : scope->entries) {
        (void)name;
        if (entry.node_kind != NodeKind::VARIABLE_NODE) {
            continue;
        }

        max_slot = std::max(max_slot, static_cast<std::size_t>(entry.offset + 1));
        found = true;
    }

    if (found) {
        state.stack_bytes = std::max(state.stack_bytes, max_slot * 8);
        state.next_slot = std::max(state.next_slot, max_slot);
    }
}

std::string named_address(const std::string& name, FunctionState& state, const Semantic::SymbolTable* scope) {
    if (auto offset = variable_offset_for_name(name, scope)) {
        return slot_address(*offset);
    }

    ensure_slot(state, name);
    return slot_address(state.named_slots[name]);
}

ValueLayout infer_result_layout(const Quadruple& quad, const FunctionState& state, const Semantic::SymbolTable* scope) {
    if (const auto* temp = std::get_if<Temp>(&quad.result)) {
        if (auto it = state.temp_layouts.find(temp->id); it != state.temp_layouts.end()) {
            return it->second;
        }
    }

    switch (quad.opcode) {
        case Opcode::CompareEq:
        case Opcode::CompareNe:
        case Opcode::CompareLt:
        case Opcode::CompareLe:
        case Opcode::CompareGt:
        case Opcode::CompareGe:
        case Opcode::JumpIfTrue:
        case Opcode::JumpIfFalse:
            return layout_for_type(TYPES.at("bool"));
        case Opcode::Assign:
            return layout_for_operand(quad.arg1, state, scope);
        case Opcode::Call:
            if (const auto* named = std::get_if<ir::NamedValue>(&quad.arg1)) {
                if (scope) {
                    if (const auto* entry = scope->lookup_global(named->name); entry && entry->type) {
                        return layout_for_type(*entry->type);
                    }
                }
            }
            return layout_for_type(TYPES.at("int"));
        default:
            return layout_for_operand(quad.arg1, state, scope);
    }
}

std::string emit_quad(const Quadruple& quad, FunctionState& state, const Semantic::SymbolTable* scope, const std::string& function_name, const CodegenConfig& config) {
    std::ostringstream out;
    if (config.emit_comments) {
        out << "  # " << ir::to_string(quad) << '\n';
    }

    auto dest_addr = [&]() -> std::string {
        if (const auto* temp = std::get_if<Temp>(&quad.result)) {
            ensure_slot(state, temp->id);
            return slot_address(state.temp_slots[temp->id]);
        }

        if (const auto* named = std::get_if<ir::NamedValue>(&quad.result)) {
            return named_address(named->name, state, scope);
        }

        return "%rax";
    };

    auto src_value = [&](const Operand& op, std::size_t scratch_index = 0) -> std::string {
        return operand_text(op, state, scope, out, scratch(scratch_index));
    };

    switch (quad.opcode) {
        case Opcode::Assign: {
            const auto dst = dest_addr();
            const auto src = src_value(quad.arg1);
            if (src.find('(') != std::string::npos && dst.find('(') != std::string::npos) {
                out << "  movq " << src << ", " << scratch(0) << '\n';
                out << "  movq " << scratch(0) << ", " << dst << '\n';
            } else {
                out << "  movq " << src << ", " << dst << '\n';
            }
            break;
        }
        case Opcode::Add:
        case Opcode::Sub:
        case Opcode::Mul:
        case Opcode::Div:
        case Opcode::Mod: {
            const auto lhs_reg = scratch(0);
            const auto rhs_reg = scratch(1);
            const auto dst = dest_addr();
            out << "  movq " << src_value(quad.arg1, 0) << ", " << lhs_reg << '\n';
            out << "  movq " << src_value(quad.arg2, 1) << ", " << rhs_reg << '\n';

            if (quad.opcode == Opcode::Div || quad.opcode == Opcode::Mod) {
                out << "  movq " << lhs_reg << ", %rax\n";
                out << "  cqo\n";
                out << "  idivq " << rhs_reg << '\n';
                out << "  movq " << (quad.opcode == Opcode::Mod ? "%rdx" : "%rax") << ", " << dst << '\n';
                break;
            }

            const char* mnemonic = quad.opcode == Opcode::Add ? "add" : quad.opcode == Opcode::Sub ? "sub" : "imul";
            out << "  " << mnemonic << "q " << rhs_reg << ", " << lhs_reg << '\n';
            out << "  movq " << lhs_reg << ", " << dst << '\n';
            break;
        }
        case Opcode::Neg:
        case Opcode::LogicalNot: {
            const auto dst = dest_addr();
            const auto reg = scratch(0);
            out << "  movq " << src_value(quad.arg1, 0) << ", " << reg << '\n';
            if (quad.opcode == Opcode::Neg) {
                out << "  negq " << reg << '\n';
            } else {
                out << "  cmp $0, " << reg << '\n';
                out << "  sete " << width_reg(reg, 8) << '\n';
                out << "  movzbq " << width_reg(reg, 8) << ", " << reg << '\n';
            }
            out << "  movq " << reg << ", " << dst << '\n';
            break;
        }
        case Opcode::CompareEq:
        case Opcode::CompareNe:
        case Opcode::CompareLt:
        case Opcode::CompareLe:
        case Opcode::CompareGt:
        case Opcode::CompareGe: {
            const auto dst = dest_addr();
            const auto lhs_reg = scratch(0);
            const auto rhs_reg = scratch(1);
            out << "  movq " << src_value(quad.arg1, 0) << ", " << lhs_reg << '\n';
            out << "  movq " << src_value(quad.arg2, 1) << ", " << rhs_reg << '\n';
            out << "  cmpq " << rhs_reg << ", " << lhs_reg << '\n';
            out << "  set" << compare_suffix(quad.opcode) << " " << width_reg(lhs_reg, 8) << '\n';
            out << "  movzbq " << width_reg(lhs_reg, 8) << ", " << lhs_reg << '\n';
            out << "  movq " << lhs_reg << ", " << dst << '\n';
            break;
        }
        case Opcode::Jump:
            out << "  jmp " << label_for(quad.result, state, function_name) << '\n';
            break;
        case Opcode::JumpIfTrue:
        case Opcode::JumpIfFalse: {
            const auto reg = scratch(0);
            out << "  movq " << src_value(quad.arg1, 0) << ", " << reg << '\n';
            out << "  cmpq $0, " << reg << '\n';
            out << "  " << (quad.opcode == Opcode::JumpIfTrue ? "jne" : "je") << " " << label_for(quad.result, state, function_name) << '\n';
            break;
        }
        case Opcode::Param: {
            const auto reg = scratch(0);
            out << "  movq " << src_value(quad.arg1, 0) << ", " << reg << '\n';
            out << "  pushq " << reg << '\n';
            break;
        }
        case Opcode::Call: {
            const auto callee = std::get_if<ir::NamedValue>(&quad.arg1) ? std::get<ir::NamedValue>(quad.arg1).name : ir::to_string(quad.arg1);
            out << "  call " << callee << '\n';
            if (const auto* count = std::get_if<ir::IntegerLiteral>(&quad.arg2); count && count->value > 0) {
                out << "  addq $" << (count->value * 8) << ", %rsp\n";
            }
            if (!std::holds_alternative<std::monostate>(quad.result)) {
                const auto dst = dest_addr();
                out << "  movq %rax, " << dst << '\n';
            }
            break;
        }
        case Opcode::Return: {
            if (!std::holds_alternative<std::monostate>(quad.arg1)) {
                const auto reg = scratch(0);
                out << "  movq " << src_value(quad.arg1, 0) << ", " << reg << '\n';
                out << "  movq " << reg << ", %rax\n";
            }
            out << "  leave\n";
            out << "  ret\n";
            break;
        }
        case Opcode::Label:
        case Opcode::Nop:
            break;
    }

    return out.str();
}

std::string render_function(const BlockFunctionIR& function, const Semantic::SymbolTable* scope, const CodegenConfig& config) {
    FunctionState state;
    const Semantic::SymbolTable* function_scope = scope;
    if (scope) {
        if (const auto* entry = scope->lookup_global(function.name); entry && entry->scope) {
            function_scope = entry->scope.get();
        }
    }

    for (const auto& block : function.blocks) {
        state.block_labels.emplace(block.name, local_label(function.name, block.name));
    }

    reserve_scope_storage(state, function_scope);

    for (const auto& block : function.blocks) {
        for (const auto& quad : block.quads) {
            if (const auto* temp = std::get_if<Temp>(&quad.result)) {
                state.temp_layouts[temp->id] = infer_result_layout(quad, state, function_scope);
                ensure_slot(state, temp->id);
            }

            if (const auto* temp = std::get_if<Temp>(&quad.arg1)) {
                ensure_slot(state, temp->id);
            }

            if (const auto* temp = std::get_if<Temp>(&quad.arg2)) {
                ensure_slot(state, temp->id);
            }

        }
    }

    if (state.stack_bytes % 16 != 0) {
        state.stack_bytes += 16 - (state.stack_bytes % 16);
    }

    std::ostringstream out;
    out << ".globl " << function.name << '\n';
    out << function.name << ":\n";
    out << "  push %rbp\n";
    out << "  mov %rsp, %rbp\n";
    if (state.stack_bytes > 0) {
        out << "  sub $" << state.stack_bytes << ", %rsp\n";
    }

    for (const auto& block : function.blocks) {
        out << state.block_labels.at(block.name) << ":\n";
        for (const auto& quad : block.quads) {
            out << emit_quad(quad, state, function_scope, function.name, config);
        }
    }

    out << "  leave\n";
    out << "  ret\n";
    return out.str();
}

std::string render_module(const BlockModuleIR& module, const Semantic::SymbolTable* scope, const CodegenConfig& config) {
    std::ostringstream out;
    out << ".text\n";
    for (const auto& function : module.functions) {
        out << render_function(function, scope, config) << '\n';
    }
    return out.str();
}

} // namespace

std::string generate_assembly(const BlockFunctionIR& function, Semantic::SymbolTable& global_scope, const CodegenConfig& config) {
    return render_function(function, &global_scope, config);
}

std::string generate_assembly(const BlockModuleIR& module, Semantic::SymbolTable& global_scope, const CodegenConfig& config) {
    return render_module(module, &global_scope, config);
}

std::string generate_assembly(const BlockFunctionIR& function, const CodegenConfig& config) {
    Semantic::SymbolTable scope{"global"};
    return generate_assembly(function, scope, config);
}

std::string generate_assembly(const BlockModuleIR& module, const CodegenConfig& config) {
    Semantic::SymbolTable scope{"global"};
    return generate_assembly(module, scope, config);
}

} // namespace backend::codegen
