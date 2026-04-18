#pragma once

#include "../../ast/Condition.h"
#include "TypeAnalysis.h"
#include "../../Support/Diagnostics/Diagnostics.h"

class TypeCheckingVisitor final : public Visitor {

    ::Semantic::SymbolTable* table;
    Diagnostics& diagnostics;
    bool expect_expression_yield = false;
    std::optional<Type> current_return_type;
    std::optional<Type> current_pipeline_input;

    void visit(BodyNode& b, ::Semantic::SymbolTable* current_table);
    void visit(ClassNode& c, ::Semantic::SymbolTable* current_table);
    void visit(ConstructorNode& c, ::Semantic::SymbolTable* current_table);
    void visit(CascadeNode& id, ::Semantic::SymbolTable* current_table);
    void visit(WhileNode& whl, ::Semantic::SymbolTable* current_table);
    void visit(ForNode& for_node, ::Semantic::SymbolTable* current_table);
    void visit(IfNode& if_node, ::Semantic::SymbolTable* current_table);
    void visit(ElseIfNode& else_if_node, ::Semantic::SymbolTable* current_table);
    void visit(ElseNode& else_node, ::Semantic::SymbolTable* current_table);
    void visit(MatchNode& match_node, ::Semantic::SymbolTable* current_table, bool require_yield);
    bool validate_bool_condition(const Condition* cond, const char* context, ::Semantic::SymbolTable* current_table);
    void report_error(std::string message);
    void report_error(std::string message, const Token& token);
    std::unique_ptr<ConstructorNode> promoteFnCallToConstructor(FnCallNode& call);

    std::unordered_set<Condition*> node_promoted;

public:

    TypeCheckingVisitor(::Semantic::SymbolTable* t, Diagnostics& d) : table(t), diagnostics(d) {}

    void visit(GlobalNode& global) override;
    void visit(VariableNode&) override;
    void visit(BinaryExpression&) override;
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
    void visit(MatchNode&) override;
    void visit(YieldNode&) override;
    void visit(UnaryIncrNode&) override {}
    void visit(DotNode&) override;
    void visit(CascadeNode& id) override;
    void visit(PipelineNode&) override;
    void visit(ReturnNode&) override;
    void visit(ConversionNode&) override {}
    void visit(IdentifierCondition&) override {}
    void visit(IntegerCondition&) override {}
    void visit(DoubleCondition&) override {}
    void visit(BoolCondition&) override {}
    void visit(StringCondition&) override {}
    void visit(CharCondition&) override {}
};
