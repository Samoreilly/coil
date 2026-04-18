#pragma once

#include "TypeAnalysisCommon.h"

static inline bool contains_placeholder(const Node& node);
static inline bool contains_placeholder(const Node* node);
static inline bool contains_placeholder(const Condition& cond);
static inline bool contains_placeholder(const Condition* cond);
static inline bool body_contains_placeholder(const BodyNode& body);

static inline const Semantic::SymbolEntry* current_class_entry(Semantic::SymbolTable* table);
static inline const Semantic::SymbolEntry* lookup_member_entry(Semantic::SymbolTable* class_scope, const Node& member);

static inline std::optional<Type> resolve_node_type(const Node& node, Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input = std::nullopt);
static inline std::optional<Type> resolve_dot_type(const DotNode& dot, Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input = std::nullopt);

static inline const YieldNode* terminal_yield_node(const BodyNode& body);
static inline std::optional<Type> terminal_node_type(const Node& node, Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input = std::nullopt);
static inline std::optional<Type> terminal_yield_type(const BodyNode& body, Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input = std::nullopt);
static inline std::optional<Type> match_expression_type(const MatchNode& match, Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input = std::nullopt);

static inline bool body_contains_yield(const BodyNode& body) {
    return terminal_yield_node(body) != nullptr;
}

static inline const Semantic::SymbolEntry* current_class_entry(Semantic::SymbolTable* table) {
    if (!table) {
        return nullptr;
    }

    for (auto* scope = table; scope && scope->parent; scope = scope->parent) {
        if (auto* owner = scope->parent->lookup_local(scope->name); owner && owner->node_kind == NodeKind::CLASS_NODE) {
            return owner;
        }

        for (const auto& [name, entry] : scope->parent->entries) {
            (void)name;
            if (entry.node_kind == NodeKind::CLASS_NODE && entry.scope && entry.scope.get() == scope->parent) {
                return &entry;
            }
        }
    }

    return nullptr;
}

static inline const Semantic::SymbolEntry* lookup_member_entry(Semantic::SymbolTable* class_scope, const Node& member) {
    if (!class_scope) {
        return nullptr;
    }

    if (auto* ident = dynamic_cast<const IdentifierCondition*>(&member)) {
        return class_scope->lookup_local(ident->token.token_value);
    }

    if (auto* call = dynamic_cast<const FnCallNode*>(&member)) {
        return class_scope->lookup_local(call->name);
    }

    return nullptr;
}
