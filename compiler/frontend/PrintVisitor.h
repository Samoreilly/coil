#pragma once

#include "../Token.h"
#include "../common/Visitor.h"
#include <iostream>
#include <string>

class PrintVisitor final : public Visitor {
public:
    PrintVisitor(std::ostream& out = std::cout) : out(out), indent_level(0) {}

    void visit(GlobalNode& global) override;
    void visit(VariableNode& v) override;
    void visit(BinaryExpression& b) override;
    void visit(FnNode& b) override;
    void visit(FnCallNode& b) override;
    void visit(BodyNode& b) override;
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

private:

    std::ostream& out;
    int indent_level;

    void print_indent() {
        for (int i = 0; i < indent_level; ++i) out << "  ";
    }

    std::string visibility_to_string(Visibility v) {
        return (v == Visibility::PUBLIC) ? "public" : "private";
    }

    std::string type_to_string(TYPE t) {
        switch (t) {
            case TYPE::STRING: return "string";
            case TYPE::INT: return "int";
            case TYPE::DOUBLE: return "double";
            case TYPE::CHAR: return "char";
            case TYPE::BOOL: return "bool";
            case TYPE::CLASS: return "class";
            case TYPE::CRATE: return "crate";
            case TYPE::VOID: return "void";
            case TYPE::PAIR: return "pair";
            default: return "unknown";
        }
    }
};
