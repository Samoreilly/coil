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

static inline bool contains_placeholder(const Node& node);
static inline bool contains_placeholder(const Node* node);
static inline bool contains_placeholder(const Condition& cond);
static inline bool contains_placeholder(const Condition* cond);
static inline bool body_contains_placeholder(const BodyNode& body);
static inline std::optional<Type> resolve_node_type(const Node& node, Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input = std::nullopt);
static inline std::optional<Type> resolve_fn_call_type(const FnCallNode& call, Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input = std::nullopt);
static inline std::optional<Type> resolve_dot_type(const DotNode& dot, Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input = std::nullopt);
static inline const YieldNode* terminal_yield_node(const BodyNode& body);
static inline std::optional<Type> terminal_yield_type(const BodyNode& body, Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input = std::nullopt);
static inline std::optional<Type> match_expression_type(const MatchNode& match, Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input = std::nullopt);

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

static inline std::optional<Type> condition_to_type(const Condition& c, Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input = std::nullopt) {
    if (dynamic_cast<const IntegerCondition*>(&c))  return TYPES.at("int");
    if (dynamic_cast<const DoubleCondition*>(&c))   return TYPES.at("f64");
    if (dynamic_cast<const BoolCondition*>(&c))     return TYPES.at("bool");
    if (dynamic_cast<const StringCondition*>(&c))   return TYPES.at("string");
    if (dynamic_cast<const CharCondition*>(&c))     return TYPES.at("char");

    if (auto* ident = dynamic_cast<const IdentifierCondition*>(&c)) {
        if (ident->token.token_value == "_") {
            return pipeline_input;
        }

        if (auto it = TYPES.find(ident->token.token_value); it != TYPES.end()) {
            return it->second;
        }

        if (ident->token.token_value == "this" && table) {
            for (auto* scope = table; scope && scope->parent; scope = scope->parent) {
                if (auto* owner = scope->parent->lookup_local(scope->name); owner && owner->node_kind == NodeKind::CLASS_NODE) {
                    if (owner->type) {
                        return *owner->type;
                    }

                    return Type{TypeCategory::CLASS, 0, false, scope->name};
                }
            }
        }

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
        const auto left = bin->left ? condition_to_type(*bin->left, table, pipeline_input) : std::nullopt;
        const auto right = bin->right ? condition_to_type(*bin->right, table, pipeline_input) : std::nullopt;

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
        return resolve_fn_call_type(*call, table, pipeline_input);
    }

    if (auto* dot = dynamic_cast<const DotNode*>(&c)) {
        return resolve_dot_type(*dot, table, pipeline_input);
    }

    if (auto* ctor = dynamic_cast<const ConstructorNode*>(&c)) {
        return Type{TypeCategory::CLASS, 0, false, ctor->name};
    }

    if (auto* ret = dynamic_cast<const ReturnNode*>(&c)) {
        return ret->ret ? condition_to_type(*ret->ret, table, pipeline_input) : std::nullopt;
    }

    if (auto* y = dynamic_cast<const YieldNode*>(&c)) {
        return y->value ? condition_to_type(*y->value, table, pipeline_input) : std::nullopt;
    }

    if (auto* match = dynamic_cast<const MatchNode*>(&c)) {
        return match_expression_type(*match, table, pipeline_input);
    }

    if (auto* pipe = dynamic_cast<const PipelineNode*>(&c)) {
        if (!pipe->right || !contains_placeholder(*pipe->right)) {
            return std::nullopt;
        }

        const auto left = pipe->left ? condition_to_type(*pipe->left, table) : std::nullopt;
        if (!left) {
            return std::nullopt;
        }

        return pipe->right ? condition_to_type(*pipe->right, table, *left) : std::nullopt;
    }

    if (auto* var = dynamic_cast<const VariableNode*>(&c)) {
        return var->name ? condition_to_type(*var->name, table, pipeline_input) : std::nullopt;
    }

    if (auto* cascade = dynamic_cast<const CascadeNode*>(&c)) {
        return cascade->name ? condition_to_type(*cascade->name, table, pipeline_input) : std::nullopt;
    }

    if (auto* unary = dynamic_cast<const UnaryIncrNode*>(&c)) {
        return unary->name ? condition_to_type(*unary->name, table, pipeline_input) : std::nullopt;
    }

    return std::nullopt;
}

static inline std::optional<Type> body_local_type(const VariableNode& var, Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input = std::nullopt) {
    if (var.type) {
        return *var.type;
    }

    if (var.init && *var.init) {
        return condition_to_type(*var.init.value(), table, pipeline_input);
    }

    return std::nullopt;
}

static inline bool contains_placeholder(const Condition& cond) {
    if (auto* ident = dynamic_cast<const IdentifierCondition*>(&cond)) {
        return ident->token.token_value == "_";
    }

    if (auto* call = dynamic_cast<const FnCallNode*>(&cond)) {
        for (const auto& arg : call->arguments) {
            if (contains_placeholder(arg.get())) {
                return true;
            }
        }
        return false;
    }

    if (auto* dot = dynamic_cast<const DotNode*>(&cond)) {
        return contains_placeholder(dot->left.get()) || contains_placeholder(dot->right.get());
    }

    if (auto* bin = dynamic_cast<const BinaryExpression*>(&cond)) {
        return contains_placeholder(bin->left.get()) || contains_placeholder(bin->right.get());
    }

    if (auto* pipe = dynamic_cast<const PipelineNode*>(&cond)) {
        return contains_placeholder(pipe->left.get()) || contains_placeholder(pipe->right.get());
    }

    if (auto* ret = dynamic_cast<const ReturnNode*>(&cond)) {
        return contains_placeholder(ret->ret.get());
    }

    if (auto* y = dynamic_cast<const YieldNode*>(&cond)) {
        return contains_placeholder(y->value.get());
    }

    if (auto* match = dynamic_cast<const MatchNode*>(&cond)) {
        if (contains_placeholder(match->input.get())) {
            return true;
        }

        for (const auto& case_item : match->cases) {
            if (contains_placeholder(case_item.pattern.get()) || (case_item.body && body_contains_placeholder(*case_item.body))) {
                return true;
            }
        }

        return false;
    }

    if (auto* var = dynamic_cast<const VariableNode*>(&cond)) {
        return contains_placeholder(var->name.get()) || contains_placeholder(var->init ? var->init->get() : nullptr);
    }

    if (auto* cascade = dynamic_cast<const CascadeNode*>(&cond)) {
        if (contains_placeholder(cascade->name.get())) {
            return true;
        }

        for (const auto& step : cascade->cascades) {
            if (step && contains_placeholder(step->condition.get())) {
                return true;
            }
        }

        return false;
    }

    if (auto* ctor = dynamic_cast<const ConstructorNode*>(&cond)) {
        return ctor->body ? body_contains_placeholder(*ctor->body) : false;
    }

    return false;
}

static inline bool contains_placeholder(const Node& node) {
    if (auto* cond = dynamic_cast<const Condition*>(&node)) {
        return contains_placeholder(*cond);
    }

    if (auto* var = dynamic_cast<const VariableNode*>(&node)) {
        if (var->name && contains_placeholder(*var->name)) {
            return true;
        }

        if (var->init && *var->init && contains_placeholder(var->init->get())) {
            return true;
        }
    }

    if (auto* body = dynamic_cast<const BodyNode*>(&node)) {
        return body_contains_placeholder(*body);
    }

    if (auto* fn = dynamic_cast<const FnNode*>(&node)) {
        return fn->body && body_contains_placeholder(*fn->body);
    }

    if (auto* cls = dynamic_cast<const ClassNode*>(&node)) {
        return cls->body && body_contains_placeholder(*cls->body);
    }

    if (auto* ctor = dynamic_cast<const ConstructorNode*>(&node)) {
        return ctor->body && body_contains_placeholder(*ctor->body);
    }

    if (auto* crate = dynamic_cast<const CrateNode*>(&node)) {
        for (const auto& var : crate->crate_vars) {
            if (var && contains_placeholder(*var)) {
                return true;
            }
        }
    }

    if (auto* global = dynamic_cast<const GlobalNode*>(&node)) {
        for (const auto& child : global->globals) {
            if (child && contains_placeholder(*child)) {
                return true;
            }
        }
    }

    if (auto* wh = dynamic_cast<const WhileNode*>(&node)) {
        if (wh->cond && wh->cond->get() && contains_placeholder(wh->cond->get())) {
            return true;
        }
        return wh->body && body_contains_placeholder(*wh->body);
    }

    if (auto* for_node = dynamic_cast<const ForNode*>(&node)) {
        if (for_node->init && *for_node->init && contains_placeholder(*for_node->init.value())) {
            return true;
        }
        if (for_node->cond && *for_node->cond && contains_placeholder(for_node->cond->get())) {
            return true;
        }
        if (for_node->incr && *for_node->incr && contains_placeholder(for_node->incr->get())) {
            return true;
        }
        return for_node->for_body && body_contains_placeholder(*for_node->for_body);
    }

    if (auto* if_node = dynamic_cast<const IfNode*>(&node)) {
        if (if_node->cond && contains_placeholder(if_node->cond.get())) {
            return true;
        }
        if (if_node->body && body_contains_placeholder(*if_node->body)) {
            return true;
        }
        for (const auto& else_if : if_node->else_ifs) {
            if (else_if && contains_placeholder(*else_if)) {
                return true;
            }
        }
        return if_node->else_node && contains_placeholder(*if_node->else_node);
    }

    if (auto* else_if = dynamic_cast<const ElseIfNode*>(&node)) {
        if (else_if->cond && contains_placeholder(else_if->cond.get())) {
            return true;
        }
        return else_if->body && body_contains_placeholder(*else_if->body);
    }

    if (auto* else_node = dynamic_cast<const ElseNode*>(&node)) {
        return else_node->body && body_contains_placeholder(*else_node->body);
    }

    return false;
}

static inline bool contains_placeholder(const Node* node) {
    return node ? contains_placeholder(*node) : false;
}

static inline bool contains_placeholder(const Condition* cond) {
    return cond ? contains_placeholder(*cond) : false;
}

static inline bool body_contains_placeholder(const BodyNode& body) {
    for (const auto& stmt : body.statements) {
        if (stmt && contains_placeholder(stmt.get())) {
            return true;
        }
    }

    return false;
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

static inline std::optional<Type> terminal_yield_type(const BodyNode& body, Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input) {
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

            auto local_type = body_local_type(*var, &local_scope, pipeline_input);
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

    return condition_to_type(*yield->value, &local_scope, pipeline_input);
}

static inline std::optional<Type> match_expression_type(const MatchNode& match, Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input) {
    if (!match.input) {
        return std::nullopt;
    }

    auto selector_type = condition_to_type(*match.input, table, pipeline_input);
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
            auto pattern_type = condition_to_type(*case_item.pattern, table, pipeline_input);
            if (!pattern_type) {
                return std::nullopt;
            }

            if (!same_type(*selector_type, *pattern_type) && !common_numeric_type(*selector_type, *pattern_type)) {
                return std::nullopt;
            }
        }

        auto case_type = case_item.body ? terminal_yield_type(*case_item.body, table, pipeline_input) : std::nullopt;
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

static inline std::optional<Type> resolve_fn_call_type(const FnCallNode& call, Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input) {
    if (!table) {
        return std::nullopt;
    }

    auto* entry = table->lookup_global(call.name);
    if (!entry || !entry->type) {
        return std::nullopt;
    }

    if (!contains_placeholder(call)) {
        return *entry->type;
    }

    if (!pipeline_input || entry->param_types.size() != call.arguments.size()) {
        return std::nullopt;
    }

    for (std::size_t i = 0; i < call.arguments.size(); ++i) {
        if (!call.arguments[i]) {
            return std::nullopt;
        }

        auto arg_type = condition_to_type(*call.arguments[i], table, pipeline_input);
        if (!arg_type || !can_implicitly_convert(*arg_type, entry->param_types[i])) {
            return std::nullopt;
        }
    }

    return *entry->type;
}

static inline std::optional<Type> resolve_node_type(const Node& node, Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input) {
    if (auto* cond = dynamic_cast<const Condition*>(&node)) {
        return condition_to_type(*cond, table, pipeline_input);
    }

    if (auto* var = dynamic_cast<const VariableNode*>(&node)) {
        if (var->type) {
            return *var->type;
        }

        if (var->init && *var->init) {
            return condition_to_type(*var->init.value(), table, pipeline_input);
        }
    }

    return std::nullopt;
}

static inline std::optional<Type> resolve_dot_type(const DotNode& dot, Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input) {
    if (!dot.left || !dot.right) {
        return std::nullopt;
    }

    auto left_type = resolve_node_type(*dot.left, table, pipeline_input);
    if (!left_type || !table) {
        return std::nullopt;
    }

    auto* class_entry = table->lookup_global(left_type->name);
    if (!class_entry || !class_entry->scope) {
        return std::nullopt;
    }

    return resolve_node_type(*dot.right, class_entry->scope.get(), pipeline_input);
}
