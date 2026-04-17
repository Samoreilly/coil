#pragma once

#include "../../ast/Node.h"
#include "Semantic.h"

#include <optional>
#include <unordered_set>

static inline bool same_type(const Type& lhs, const Type& rhs) {
    return lhs.type == rhs.type &&
        lhs.bit_width == rhs.bit_width &&
        lhs.is_signed == rhs.is_signed &&
        lhs.name == rhs.name;
}

static inline bool is_numeric_type(const Type& type) {
    return type.type == TypeCategory::INT || type.type == TypeCategory::FLOAT;
}

static inline bool type_allows_ordering(const Type& type) {
    return is_numeric_type(type) || type.type == TypeCategory::CHAR;
}

static inline Type parameter_type(const std::unique_ptr<Parameter>& param) {
    if (!param) {
        return {TypeCategory::VOID, 0, false, "void"};
    }

    if (std::holds_alternative<Type>(param->type)) {
        return std::get<Type>(param->type);
    }

    return {TypeCategory::CLASS, 0, false, std::get<std::string>(param->type)};
}

static inline std::vector<Type> parameter_types(const std::vector<std::unique_ptr<Parameter>>& params) {
    std::vector<Type> types;
    types.reserve(params.size());

    for (const auto& param : params) {
        types.push_back(parameter_type(param));
    }

    return types;
}

static inline bool can_implicitly_convert(const Type& from, const Type& to) {
    if (same_type(from, to)) {
        return true;
    }

    if (from.type == TypeCategory::INT && to.type == TypeCategory::INT) {
        return from.is_signed == to.is_signed && from.bit_width <= to.bit_width;
    }

    if (from.type == TypeCategory::INT && to.type == TypeCategory::FLOAT) {
        return true;
    }

    if (from.type == TypeCategory::FLOAT && to.type == TypeCategory::FLOAT) {
        return from.bit_width <= to.bit_width;
    }

    return false;
}

static inline std::optional<Type> common_numeric_type(const Type& lhs, const Type& rhs) {
    if (!is_numeric_type(lhs) || !is_numeric_type(rhs)) {
        return std::nullopt;
    }

    if (same_type(lhs, rhs)) {
        return lhs;
    }

    if (lhs.type == TypeCategory::FLOAT || rhs.type == TypeCategory::FLOAT) {
        if (lhs.type == TypeCategory::FLOAT && rhs.type == TypeCategory::FLOAT) {
            if (lhs.bit_width >= rhs.bit_width) return lhs;
            return rhs;
        }

        if (lhs.type == TypeCategory::FLOAT && rhs.type == TypeCategory::INT) {
            return lhs;
        }

        if (lhs.type == TypeCategory::INT && rhs.type == TypeCategory::FLOAT) {
            return rhs;
        }

        return std::nullopt;
    }

    if (lhs.type == TypeCategory::INT && rhs.type == TypeCategory::INT) {
        if (lhs.is_signed != rhs.is_signed) {
            return std::nullopt;
        }

        if (lhs.bit_width >= rhs.bit_width) return lhs;
        return rhs;
    }

    return std::nullopt;
}

static inline std::optional<Type> condition_to_type(const Condition& c, Semantic::SymbolTable* table) {
    if (dynamic_cast<const IntegerCondition*>(&c))  return TYPES.at("int");
    if (dynamic_cast<const DoubleCondition*>(&c))   return TYPES.at("f64");
    if (dynamic_cast<const BoolCondition*>(&c))     return TYPES.at("bool");
    if (dynamic_cast<const StringCondition*>(&c))   return TYPES.at("string");
    if (dynamic_cast<const CharCondition*>(&c))     return TYPES.at("char");

    if (auto* ident = dynamic_cast<const IdentifierCondition*>(&c)) {
        if (ident->token.token_value == "true" || ident->token.token_value == "false") {
            return TYPES.at("bool");
        }

        if (table) {
            if (auto* entry = table->lookup_local(ident->token.token_value)) {
                if (entry->type) return *entry->type;
            }

            if (auto* entry = table->lookup_global(ident->token.token_value)) {
                if (entry->type) return *entry->type;
            }
        }
    }

    if (auto* conv = dynamic_cast<const ConversionNode*>(&c)) {
        return conv->to_type;
    }

    if (auto* bin = dynamic_cast<const BinaryExpression*>(&c)) {
        static const std::unordered_set<std::string> comparison_ops = {
            "<", ">", "<=", ">=", "==", "!="
        };
        static const std::unordered_set<std::string> arithmetic_ops = {
            "+", "-", "*", "/"
        };

        const auto left = bin->left ? condition_to_type(*bin->left, table) : std::nullopt;
        const auto right = bin->right ? condition_to_type(*bin->right, table) : std::nullopt;

        if (comparison_ops.count(bin->op) != 0) {
            if (!left || !right) {
                return std::nullopt;
            }

            if (bin->op == "<" || bin->op == ">" || bin->op == "<=" || bin->op == ">=") {
                if (same_type(*left, *right) && type_allows_ordering(*left)) {
                    return TYPES.at("bool");
                }

                if (common_numeric_type(*left, *right)) {
                    return TYPES.at("bool");
                }

                return std::nullopt;
            }

            if (same_type(*left, *right)) {
                return TYPES.at("bool");
            }

            if (common_numeric_type(*left, *right)) {
                return TYPES.at("bool");
            }

            return std::nullopt;
        }

        if (arithmetic_ops.count(bin->op) != 0) {
            if (!left || !right) {
                return std::nullopt;
            }

            if (auto promoted = common_numeric_type(*left, *right)) {
                return promoted;
            }

            return std::nullopt;
        }

        if (left) {
            return left;
        }

        if (right) {
            return right;
        }
    }

    if (auto* call = dynamic_cast<const FnCallNode*>(&c)) {
        if (table) {
            if (auto* entry = table->lookup_global(call->name)) {
                if (entry->type) return *entry->type;
            }
        }
    }

    if (auto* ctor = dynamic_cast<const ConstructorNode*>(&c)) {
        return Type{TypeCategory::CLASS, 0, false, ctor->name};
    }

    if (auto* ret = dynamic_cast<const ReturnNode*>(&c)) {
        return ret->ret ? condition_to_type(*ret->ret, table) : std::nullopt;
    }

    if (auto* var = dynamic_cast<const VariableNode*>(&c)) {
        return var->name ? condition_to_type(*var->name, table) : std::nullopt;
    }

    if (auto* cascade = dynamic_cast<const CascadeNode*>(&c)) {
        return cascade->name ? condition_to_type(*cascade->name, table) : std::nullopt;
    }

    if (auto* unary = dynamic_cast<const UnaryIncrNode*>(&c)) {
        return unary->name ? condition_to_type(*unary->name, table) : std::nullopt;
    }

    return std::nullopt;
}
