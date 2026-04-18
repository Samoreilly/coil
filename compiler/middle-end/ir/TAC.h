#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace ir {

struct Temp final {
    std::size_t id = 0;
};

struct Label final {
    std::size_t id = 0;
    std::string name;
};

struct NamedValue final {
    std::string name;
};

struct IntegerLiteral final {
    std::int64_t value = 0;
};

struct FloatingLiteral final {
    double value = 0.0;
};

struct BooleanLiteral final {
    bool value = false;
};

struct CharacterLiteral final {
    char value = '\0';
};

struct StringLiteral final {
    std::string value;
};

using Operand = std::variant<
    std::monostate,
    Temp,
    Label,
    NamedValue,
    IntegerLiteral,
    FloatingLiteral,
    BooleanLiteral,
    CharacterLiteral,
    StringLiteral>;

enum class Opcode {
    Nop,
    Assign,
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Neg,
    LogicalNot,
    CompareEq,
    CompareNe,
    CompareLt,
    CompareLe,
    CompareGt,
    CompareGe,
    Jump,
    JumpIfTrue,
    JumpIfFalse,
    Param,
    Call,
    Return,
    Label
};

struct Quadruple final {
    Opcode opcode = Opcode::Nop;
    Operand result{};
    Operand arg1{};
    Operand arg2{};

    bool is_terminator() const;
};

struct FunctionIR final {
    std::string name;
    std::vector<Quadruple> quads;
    std::size_t next_temp = 0;
    std::size_t next_label = 0;
};

struct ModuleIR final {
    std::vector<FunctionIR> functions;
};

class FunctionBuilder final {
public:
    explicit FunctionBuilder(std::string name);

    FunctionBuilder(const FunctionBuilder&) = delete;
    FunctionBuilder& operator=(const FunctionBuilder&) = delete;
    FunctionBuilder(FunctionBuilder&&) noexcept = default;
    FunctionBuilder& operator=(FunctionBuilder&&) noexcept = default;

    Temp new_temp();
    Label new_label(std::string_view hint = {});

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

    FunctionIR finish();
    const FunctionIR& function() const;

private:
    FunctionIR function_;
};

Temp temp(std::size_t id);
Label label(std::size_t id, std::string name = {});
NamedValue named(std::string name);
IntegerLiteral integer(std::int64_t value);
FloatingLiteral floating(double value);
BooleanLiteral boolean(bool value);
CharacterLiteral character(char value);
StringLiteral string_literal(std::string value);

bool is_empty(const Operand& operand);
bool is_terminator(Opcode opcode);
bool is_terminator(const Quadruple& quad);

std::string to_string(const Operand& operand);
std::string to_string(Opcode opcode);
std::string to_string(const Quadruple& quad);
std::string to_string(const FunctionIR& function);
std::string to_string(const ModuleIR& module);

std::string label_name(const Label& label);

} // namespace ir
