#pragma once

#include "../../ast/Node.h"
#include "Semantic.h"

#include <optional>

static inline bool same_type(const Type& lhs, const Type& rhs) {
    return lhs.type == rhs.type &&
        lhs.bit_width == rhs.bit_width &&
        lhs.is_signed == rhs.is_signed &&
        lhs.name == rhs.name;
}

static inline const YieldNode* terminal_yield_node(const BodyNode& body);
static inline std::optional<Type> terminal_yield_type(const BodyNode& body, Semantic::SymbolTable* table);
static inline std::optional<Type> match_expression_type(const MatchNode& match, Semantic::SymbolTable* table);

static inline bool body_contains_yield(const BodyNode& body) {
    return terminal_yield_node(body) != nullptr;
}

static inline bool is_numeric_type(const Type& type) {
    return type.type == TypeCategory::INT || type.type == TypeCategory::FLOAT;
}

static inline bool type_allows_ordering(const Type& type) {
    return is_numeric_type(type) || type.type == TypeCategory::CHAR;
}

static inline bool is_arithmetic_operator(const std::string& op) {
    return op == "+" || op == "-" || op == "*" || op == "/";
}

static inline bool is_comparison_operator(const std::string& op) {
    return op == "<" || op == ">" || op == "<=" || op == ">=" || op == "==" || op == "!=";
}

static inline bool is_ordering_comparison_operator(const std::string& op) {
    return op == "<" || op == ">" || op == "<=" || op == ">=";
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

static inline const YieldNode* terminal_yield_node(const BodyNode& body);
static inline std::optional<Type> terminal_yield_type(const BodyNode& body, Semantic::SymbolTable* table);
static inline std::optional<Type> match_expression_type(const MatchNode& match, Semantic::SymbolTable* table);

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
        const auto left = bin->left ? condition_to_type(*bin->left, table) : std::nullopt;
        const auto right = bin->right ? condition_to_type(*bin->right, table) : std::nullopt;

        if (is_comparison_operator(bin->op)) {
            if (!left || !right) {
                return std::nullopt;
            }

            if (is_ordering_comparison_operator(bin->op)) {
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

        if (is_arithmetic_operator(bin->op)) {
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

    if (auto* y = dynamic_cast<const YieldNode*>(&c)) {
        return y->value ? condition_to_type(*y->value, table) : std::nullopt;
    }

    if (auto* match = dynamic_cast<const MatchNode*>(&c)) {
        return match_expression_type(*match, table);
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

static inline std::optional<Type> body_local_type(const VariableNode& var, Semantic::SymbolTable* table) {
    if (var.type) {
        return *var.type;
    }

    if (var.init && *var.init) {
        return condition_to_type(*var.init.value(), table);
    }

    return std::nullopt;
}

static inline const YieldNode* terminal_yield_node(const BodyNode& body) {
    for (auto it = body.statements.rbegin(); it != body.statements.rend(); ++it) {
        if (!*it) {
            continue;
        }

        if (auto* yield = dynamic_cast<const YieldNode*>(it->get())) {
            return yield;
        }
    }

    return nullptr;
}

static inline std::optional<Type> terminal_yield_type(const BodyNode& body, Semantic::SymbolTable* table) {
    const auto* yield = terminal_yield_node(body);
    if (!yield || !yield->value) {
        return std::nullopt;
    }

    Semantic::SymbolTable local_scope("match_arm");
    local_scope.parent = table;

    for (const auto& stmt : body.statements) {
        if (!stmt) {
            continue;
        }

        if (stmt.get() == yield) {
            break;
        }

        if (auto* var = dynamic_cast<const VariableNode*>(stmt.get())) {
            auto* ident = dynamic_cast<const IdentifierCondition*>(var->name.get());
            if (!ident) {
                continue;
            }

            auto local_type = body_local_type(*var, &local_scope);
            if (!local_type) {
                continue;
            }

            Semantic::SymbolEntry entry(
                NodeKind::VARIABLE_NODE,
                var->vis,
                var->access,
                *local_type,
                {},
                {},
                0,
                ident->token
            );

            local_scope.insert(ident->token.token_value, entry);
        }
    }

    return condition_to_type(*yield->value, &local_scope);
}

static inline std::optional<Type> match_expression_type(const MatchNode& match, Semantic::SymbolTable* table) {
    if (!match.input) {
        return std::nullopt;
    }

    auto selector_type = condition_to_type(*match.input, table);
    if (!selector_type) {
        return std::nullopt;
    }

    bool any_yield = false;
    bool all_yield = true;
    std::optional<Type> result_type;

    for (const auto& case_item : match.cases) {
        if (!case_item.pattern) {
            return std::nullopt;
        }

        if (auto* ident = dynamic_cast<const IdentifierCondition*>(case_item.pattern.get()); ident && ident->token.token_value == "_") {
            // wildcard arm: no selector type check
        } else {
            auto pattern_type = condition_to_type(*case_item.pattern, table);
            if (!pattern_type) {
                return std::nullopt;
            }

            if (!same_type(*selector_type, *pattern_type) && !common_numeric_type(*selector_type, *pattern_type)) {
                return std::nullopt;
            }
        }

        auto case_type = case_item.body ? terminal_yield_type(*case_item.body, table) : std::nullopt;
        if (!case_type) {
            all_yield = false;
            continue;
        }

        any_yield = true;

        if (!result_type) {
            result_type = case_type;
            continue;
        }

        if (same_type(*result_type, *case_type)) {
            continue;
        }

        if (auto promoted = common_numeric_type(*result_type, *case_type)) {
            result_type = *promoted;
            continue;
        }

        return std::nullopt;
    }

    if (!any_yield) {
        return TYPES.at("void");
    }

    if (!all_yield) {
        return std::nullopt;
    }

    return result_type ? result_type : TYPES.at("void");
}
