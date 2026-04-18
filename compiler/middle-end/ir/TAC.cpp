#include "TAC.h"

#include <sstream>
#include <utility>

namespace ir {
namespace {

template <typename... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

template <typename... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

std::string op_to_infix(Opcode opcode) {
    switch (opcode) {
        case Opcode::Add: return "+";
        case Opcode::Sub: return "-";
        case Opcode::Mul: return "*";
        case Opcode::Div: return "/";
        case Opcode::Mod: return "%";
        case Opcode::CompareEq: return "==";
        case Opcode::CompareNe: return "!=";
        case Opcode::CompareLt: return "<";
        case Opcode::CompareLe: return "<=";
        case Opcode::CompareGt: return ">";
        case Opcode::CompareGe: return ">=";
        default: return "?";
    }
}

} // namespace

bool Quadruple::is_terminator() const {
    return ir::is_terminator(opcode);
}

FunctionBuilder::FunctionBuilder(std::string name) {
    function_.name = std::move(name);
}

Temp FunctionBuilder::new_temp() {
    return Temp{function_.next_temp++};
}

Label FunctionBuilder::new_label(std::string_view hint) {
    const auto id = function_.next_label++;
    std::string name = hint.empty() ? "L" + std::to_string(id) : std::string(hint) + "_" + std::to_string(id);
    return Label{id, std::move(name)};
}

Quadruple& FunctionBuilder::emit(Opcode opcode, Operand result, Operand arg1, Operand arg2) {
    function_.quads.push_back(Quadruple{opcode, std::move(result), std::move(arg1), std::move(arg2)});
    return function_.quads.back();
}

Quadruple& FunctionBuilder::emit_assign(Operand destination, Operand source) {
    return emit(Opcode::Assign, std::move(destination), std::move(source));
}

Quadruple& FunctionBuilder::emit_binary(Opcode opcode, Operand destination, Operand lhs, Operand rhs) {
    return emit(opcode, std::move(destination), std::move(lhs), std::move(rhs));
}

Quadruple& FunctionBuilder::emit_unary(Opcode opcode, Operand destination, Operand value) {
    return emit(opcode, std::move(destination), std::move(value));
}

Quadruple& FunctionBuilder::emit_jump(Label target) {
    return emit(Opcode::Jump, std::move(target));
}

Quadruple& FunctionBuilder::emit_jump_if_true(Operand condition, Label target) {
    return emit(Opcode::JumpIfTrue, std::move(target), std::move(condition));
}

Quadruple& FunctionBuilder::emit_jump_if_false(Operand condition, Label target) {
    return emit(Opcode::JumpIfFalse, std::move(target), std::move(condition));
}

Quadruple& FunctionBuilder::emit_param(Operand value) {
    return emit(Opcode::Param, {}, std::move(value));
}

Quadruple& FunctionBuilder::emit_call(Operand destination, Operand callee, std::size_t argument_count) {
    return emit(Opcode::Call, std::move(destination), std::move(callee), integer(static_cast<std::int64_t>(argument_count)));
}

Quadruple& FunctionBuilder::emit_return(Operand value) {
    return emit(Opcode::Return, {}, std::move(value));
}

Quadruple& FunctionBuilder::emit_label(Label label_value) {
    return emit(Opcode::Label, std::move(label_value));
}

FunctionIR FunctionBuilder::finish() {
    return std::move(function_);
}

const FunctionIR& FunctionBuilder::function() const {
    return function_;
}

Temp temp(std::size_t id) {
    return Temp{id};
}

Label label(std::size_t id, std::string name) {
    return Label{id, std::move(name)};
}

NamedValue named(std::string name) {
    return NamedValue{std::move(name)};
}

IntegerLiteral integer(std::int64_t value) {
    return IntegerLiteral{value};
}

FloatingLiteral floating(double value) {
    return FloatingLiteral{value};
}

BooleanLiteral boolean(bool value) {
    return BooleanLiteral{value};
}

CharacterLiteral character(char value) {
    return CharacterLiteral{value};
}

StringLiteral string_literal(std::string value) {
    return StringLiteral{std::move(value)};
}

bool is_empty(const Operand& operand) {
    return std::holds_alternative<std::monostate>(operand);
}

bool is_terminator(Opcode opcode) {
    switch (opcode) {
        case Opcode::Jump:
        case Opcode::JumpIfTrue:
        case Opcode::JumpIfFalse:
        case Opcode::Return:
            return true;
        default:
            return false;
    }
}

bool is_terminator(const Quadruple& quad) {
    return is_terminator(quad.opcode);
}

std::string label_name(const Label& label_value) {
    if (!label_value.name.empty()) {
        return label_value.name;
    }

    return "L" + std::to_string(label_value.id);
}

std::string to_string(const Operand& operand) {
    return std::visit(Overloaded{
        [](const std::monostate&) { return std::string{}; },
        [](const Temp& value) { return "t" + std::to_string(value.id); },
        [](const Label& value) { return label_name(value); },
        [](const NamedValue& value) { return value.name; },
        [](const IntegerLiteral& value) { return std::to_string(value.value); },
        [](const FloatingLiteral& value) {
            std::ostringstream out;
            out << value.value;
            return out.str();
        },
        [](const BooleanLiteral& value) { return value.value ? std::string{"true"} : std::string{"false"}; },
        [](const CharacterLiteral& value) { return std::string(1, value.value); },
        [](const StringLiteral& value) { return '"' + value.value + '"'; }
    }, operand);
}

std::string to_string(Opcode opcode) {
    switch (opcode) {
        case Opcode::Nop: return "nop";
        case Opcode::Assign: return "assign";
        case Opcode::Add: return "add";
        case Opcode::Sub: return "sub";
        case Opcode::Mul: return "mul";
        case Opcode::Div: return "div";
        case Opcode::Mod: return "mod";
        case Opcode::Neg: return "neg";
        case Opcode::LogicalNot: return "not";
        case Opcode::CompareEq: return "cmpeq";
        case Opcode::CompareNe: return "cmpne";
        case Opcode::CompareLt: return "cmplt";
        case Opcode::CompareLe: return "cmple";
        case Opcode::CompareGt: return "cmpgt";
        case Opcode::CompareGe: return "cmpge";
        case Opcode::Jump: return "jump";
        case Opcode::JumpIfTrue: return "jump_if_true";
        case Opcode::JumpIfFalse: return "jump_if_false";
        case Opcode::Param: return "param";
        case Opcode::Call: return "call";
        case Opcode::Return: return "return";
        case Opcode::Label: return "label";
    }

    return "unknown";
}

std::string to_string(const Quadruple& quad) {
    std::ostringstream out;

    switch (quad.opcode) {
        case Opcode::Assign:
            out << to_string(quad.result) << " = " << to_string(quad.arg1);
            break;
        case Opcode::Add:
        case Opcode::Sub:
        case Opcode::Mul:
        case Opcode::Div:
        case Opcode::Mod:
        case Opcode::CompareEq:
        case Opcode::CompareNe:
        case Opcode::CompareLt:
        case Opcode::CompareLe:
        case Opcode::CompareGt:
        case Opcode::CompareGe:
            out << to_string(quad.result) << " = " << to_string(quad.arg1)
                << ' ' << op_to_infix(quad.opcode) << ' ' << to_string(quad.arg2);
            break;
        case Opcode::Neg:
            out << to_string(quad.result) << " = -" << to_string(quad.arg1);
            break;
        case Opcode::LogicalNot:
            out << to_string(quad.result) << " = !" << to_string(quad.arg1);
            break;
        case Opcode::Jump:
            out << "jump " << to_string(quad.result);
            break;
        case Opcode::JumpIfTrue:
            out << "if " << to_string(quad.arg1) << " goto " << to_string(quad.result);
            break;
        case Opcode::JumpIfFalse:
            out << "ifFalse " << to_string(quad.arg1) << " goto " << to_string(quad.result);
            break;
        case Opcode::Param:
            out << "param " << to_string(quad.arg1);
            break;
        case Opcode::Call: {
            const auto count = std::get<IntegerLiteral>(quad.arg2).value;
            const auto destination = to_string(quad.result);
            if (!destination.empty()) {
                out << destination << " = ";
            }
            out << "call " << to_string(quad.arg1) << ", " << count;
            break;
        }
        case Opcode::Return:
            if (is_empty(quad.arg1)) {
                out << "return";
            } else {
                out << "return " << to_string(quad.arg1);
            }
            break;
        case Opcode::Label:
            out << to_string(quad.result) << ':';
            break;
        case Opcode::Nop:
            out << "nop";
            break;
    }

    return out.str();
}

std::string to_string(const FunctionIR& function) {
    std::ostringstream out;
    out << "function " << function.name << "\n";
    for (const auto& quad : function.quads) {
        out << "  " << to_string(quad) << '\n';
    }
    return out.str();
}

std::string to_string(const ModuleIR& module) {
    std::ostringstream out;
    for (const auto& function : module.functions) {
        out << to_string(function);
    }
    return out.str();
}

} // namespace ir
