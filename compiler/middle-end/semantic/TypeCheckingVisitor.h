#pragma once

#include "../../ast/Node.h"
#include "../../ast/Condition.h"
#include "../../Support/Diagnostics/Diagnostics.h"
#include "Semantic.h"
#include <unordered_set>

static std::optional<Type> condition_to_type(const Condition& c, Semantic::SymbolTable* table) {
    if (dynamic_cast<const IntegerCondition*>(&c))  return TYPES.at("int");
    if (dynamic_cast<const DoubleCondition*>(&c))   return TYPES.at("f64");
    if (dynamic_cast<const BoolCondition*>(&c))     return TYPES.at("bool");
    if (dynamic_cast<const StringCondition*>(&c))    return TYPES.at("string");
    if (dynamic_cast<const CharCondition*>(&c))      return TYPES.at("char");
    if (auto* id = dynamic_cast<const IdentifierCondition*>(&c)) {
        if (table) {
            if (auto* entry = table->lookup_global(id->token.token_value)) {
                if (entry->type) return *entry->type;
            }
        }
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

    void report_error(std::string message);
    void report_error(std::string message, const Token& token);
    std::unique_ptr<ConstructorNode> promoteFnCallToConstructor(FnCallNode& call);

    std::unordered_set<Condition*> node_promoted;
        
public:

    TypeCheckingVisitor(::Semantic::SymbolTable* t, Diagnostics& d) : table(t), diagnostics(d) {}

    void visit(GlobalNode& global) override;
    void visit(VariableNode&) override;
    void visit(BinaryExpression&) override {}
    void visit(FnNode&) override {}
    void visit(FnCallNode& b) override;
    void visit(ConstructorNode& c) override;
    void visit(BodyNode& b) override;
    void visit(ClassNode& c) override;
    void visit(CrateNode&) override {}
    void visit(WhileNode&) override {}
    void visit(ForNode&) override {}
    void visit(IfNode&) override {}
    void visit(ElseIfNode&) override {}
    void visit(ElseNode&) override {}
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
