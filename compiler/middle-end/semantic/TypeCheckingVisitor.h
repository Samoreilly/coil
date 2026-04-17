#pragma once

#include "../../ast/Node.h"
#include "../../ast/Condition.h"
#include "../../Support/Diagnostics/Diagnostics.h"
#include "Semantic.h"
#include <unordered_set>

static bool type_allows_ordering(const Type& t) {
    return t.type == TypeCategory::INT || t.type == TypeCategory::FLOAT || t.type == TypeCategory::CHAR;
}

static bool same_type(const Type& lhs, const Type& rhs) {
    return lhs.type == rhs.type &&
        lhs.bit_width == rhs.bit_width &&
        lhs.is_signed == rhs.is_signed &&
        lhs.name == rhs.name;
}

static std::optional<Type> condition_to_type(const Condition& c, Semantic::SymbolTable* table) {
    if (dynamic_cast<const IntegerCondition*>(&c))  return TYPES.at("int");
    if (dynamic_cast<const DoubleCondition*>(&c))   return TYPES.at("f64");
    if (dynamic_cast<const BoolCondition*>(&c))     return TYPES.at("bool");
    if (dynamic_cast<const StringCondition*>(&c))    return TYPES.at("string");
    if (dynamic_cast<const CharCondition*>(&c))      return TYPES.at("char");
    if (auto* ident = dynamic_cast<const IdentifierCondition*>(&c)) {
        if (ident->token.token_value == "true" || ident->token.token_value == "false") {
            return TYPES.at("bool");
        }
    }
    if (auto* bin = dynamic_cast<const BinaryExpression*>(&c)) {
        static const std::unordered_set<std::string> comparison_ops = {
            "<", ">", "<=", ">=", "==", "!="
        };

        if (comparison_ops.count(bin->op) != 0) {
            if (!bin->left || !bin->right) {
                return std::nullopt;
            }

            auto left = condition_to_type(*bin->left, table);
            auto right = condition_to_type(*bin->right, table);
            if (!left || !right) {
                return std::nullopt;
            }

            if (bin->op == "<" || bin->op == ">" || bin->op == "<=" || bin->op == ">=") {
                if (!same_type(*left, *right) || !type_allows_ordering(*left)) {
                    return std::nullopt;
                }
            } else {
                if (!same_type(*left, *right)) {
                    return std::nullopt;
                }
            }

            return TYPES.at("bool");
        }

        if (bin->left) {
            if (auto left = condition_to_type(*bin->left, table)) {
                return left;
            }
        }

        if (bin->right) {
            if (auto right = condition_to_type(*bin->right, table)) {
                return right;
            }
        }
    }
    if (auto* id = dynamic_cast<const IdentifierCondition*>(&c)) {
        if (table) {
            if (auto* entry = table->lookup_global(id->token.token_value)) {
                if (entry->type) return *entry->type;
            }
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
    return std::nullopt;
}

class TypeCheckingVisitor final : public Visitor {

    ::Semantic::SymbolTable* table;
    Diagnostics& diagnostics;

    void visit(BodyNode& b, ::Semantic::SymbolTable* current_table);
    void visit(ClassNode& c, ::Semantic::SymbolTable* current_table);
    void visit(ConstructorNode& c, ::Semantic::SymbolTable* current_table);
    void visit(CascadeNode& id, ::Semantic::SymbolTable* current_table);
    void visit(WhileNode& whl, ::Semantic::SymbolTable* current_table);
    void visit(ForNode& for_node, ::Semantic::SymbolTable* current_table);
    void visit(IfNode& if_node, ::Semantic::SymbolTable* current_table);
    void visit(ElseIfNode& else_if_node, ::Semantic::SymbolTable* current_table);
    void visit(ElseNode& else_node, ::Semantic::SymbolTable* current_table);
    bool validate_bool_condition(const Condition* cond, const char* context, ::Semantic::SymbolTable* current_table);
    void report_error(std::string message);
    void report_error(std::string message, const Token& token);
    std::unique_ptr<ConstructorNode> promoteFnCallToConstructor(FnCallNode& call);

    std::unordered_set<Condition*> node_promoted;
        
public:

    TypeCheckingVisitor(::Semantic::SymbolTable* t, Diagnostics& d) : table(t), diagnostics(d) {}

    void visit(GlobalNode& global) override;
    void visit(VariableNode&) override;
    void visit(BinaryExpression&) override {}
    void visit(FnNode& b) override;
    void visit(FnCallNode& b) override;
    void visit(ConstructorNode& c) override;
    void visit(BodyNode& b) override;
    void visit(ClassNode& c) override;
    void visit(CrateNode&) override {}
    void visit(WhileNode&) override;
    void visit(ForNode&) override;
    void visit(IfNode&) override;
    void visit(ElseIfNode&) override;
    void visit(ElseNode&) override;
    void visit(MatchNode&) override {}
    void visit(UnaryIncrNode&) override {}
    void visit(DotNode&) override {}
    void visit(CascadeNode& id) override;
    void visit(PipelineNode&) override {}
    void visit(ReturnNode&) override {}
    void visit(IdentifierCondition&) override {}
    void visit(IntegerCondition&) override {}
    void visit(DoubleCondition&) override {}
    void visit(BoolCondition&) override {}
    void visit(StringCondition&) override {}
    void visit(CharCondition&) override {}
};
