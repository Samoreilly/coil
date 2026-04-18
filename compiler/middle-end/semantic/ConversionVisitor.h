#pragma once

#include "TypeAnalysis.h"
#include "../../Support/Diagnostics/Diagnostics.h"

class ConversionVisitor final : public Visitor {
    ::Semantic::SymbolTable* table;
    Diagnostics& diagnostics;

    void visit(BodyNode& b, ::Semantic::SymbolTable* current_table);
    void visit(ClassNode& c, ::Semantic::SymbolTable* current_table);
    void visit(ConstructorNode& c, ::Semantic::SymbolTable* current_table);
    std::unique_ptr<Condition> convert_if_implicit(std::unique_ptr<Condition> expr, const Type& from, const Type& to);
    void convert_function_arguments(FnCallNode& call);
    void convert_binary(BinaryExpression& b);
public:
    ConversionVisitor(::Semantic::SymbolTable* t, Diagnostics& d) : table(t), diagnostics(d) {}

    void visit(GlobalNode& global) override;
    void visit(VariableNode& v) override;
    void visit(BinaryExpression& b) override;
    void visit(FnNode& b) override;
    void visit(FnCallNode& b) override;
    void visit(ConstructorNode& c) override;
    void visit(BodyNode& b) override;
    void visit(ClassNode& c) override;
    void visit(CrateNode& cr) override;
    void visit(WhileNode& b) override;
    void visit(ForNode& b) override;
    void visit(IfNode& b) override;
    void visit(ElseIfNode& b) override;
    void visit(ElseNode& b) override;
    void visit(MatchNode& b) override;
    void visit(UnaryIncrNode& b) override;
    void visit(DotNode& b) override;
    void visit(CascadeNode& id) override;
    void visit(PipelineNode& id) override;
    void visit(ReturnNode& b) override;
    void visit(YieldNode& b) override;
    void visit(ConversionNode& b) override;
    void visit(IdentifierCondition& b) override;
    void visit(IntegerCondition& id) override;
    void visit(DoubleCondition& id) override;
    void visit(BoolCondition& id) override;
    void visit(StringCondition& id) override;
    void visit(CharCondition& id) override;
};
