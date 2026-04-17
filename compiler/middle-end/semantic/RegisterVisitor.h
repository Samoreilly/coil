#pragma once

#include "../../ast/Node.h"
#include "../../ast/Condition.h"
#include "../../Support/Diagnostics/Diagnostics.h"
#include "Semantic.h"

/*
Responbility: filling the symbol table, catching duplicates
*/

class RegisterVisitor final : public Visitor {

    ::Semantic::SymbolTable* table;
    Diagnostics& diagnostics;
    ::Semantic::SymbolTable* active_table;
    int active_offset = 0;

    void visit(GlobalNode& global, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(VariableNode& v, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(BinaryExpression& b, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(FnNode& b, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(FnCallNode& b, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(ConstructorNode& c, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(BodyNode& b, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(ClassNode& c, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(CrateNode& cr, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(WhileNode& b, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(ForNode& b, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(IfNode& b, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(ElseIfNode& b, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(ElseNode& b, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(MatchNode& b, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(UnaryIncrNode& b, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(DotNode& b, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(CascadeNode& id, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(PipelineNode& id, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(ReturnNode& b, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(ConversionNode& b, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(IdentifierCondition& b, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(IntegerCondition& id, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(DoubleCondition& id, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(BoolCondition& id, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(StringCondition& id, ::Semantic::SymbolTable* current_table, int& current_offset);
    void visit(CharCondition& id, ::Semantic::SymbolTable* current_table, int& current_offset);
public:

    RegisterVisitor(::Semantic::SymbolTable* t, Diagnostics& d) : table(t), diagnostics(d), active_table(t) {}

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
    void visit(ConversionNode& b) override;
    void visit(IdentifierCondition& b) override;
    void visit(IntegerCondition& id) override;
    void visit(DoubleCondition& id) override;
    void visit(BoolCondition& id) override;
    void visit(StringCondition& id) override;
    void visit(CharCondition& id) override;

};
