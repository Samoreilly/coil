#pragma once

#include "../../ast/Node.h"
#include "../../ast/Condition.h"
#include "../../Support/Diagnostics/Diagnostics.h"
#include "Semantic.h"
#include <unordered_set>

class TypeCheckingVisitor final : public Visitor {

    ::Semantic::SymbolTable* table;
    Diagnostics& diagnostics;

    void report_error(std::string message);
    void report_error(std::string message, const Token& token);
    std::unique_ptr<ConstructorNode> promoteFnCallToConstructor(FnCallNode& call);

    std::unordered_set<Condition*> node_promoted;
        
public:

    TypeCheckingVisitor(::Semantic::SymbolTable* t, Diagnostics& d) : table(t), diagnostics(d) {}

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
    void visit(IdentifierCondition& b) override;
    void visit(IntegerCondition& id) override;
    void visit(DoubleCondition& id) override;
    void visit(BoolCondition& id) override;
    void visit(StringCondition& id) override;
    void visit(CharCondition& id) override;
};
