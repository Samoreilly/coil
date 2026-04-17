#pragma once

#include "../../frontend/lexer/Token.h"
#include "../../ast/Condition.h"
#include "Visitor.h"
#include <variant>
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

private:

    std::ostream& out;
    int indent_level;

    void print_indent() {
        for (int i = 0; i < indent_level; ++i) out << "  ";
    }

    std::string visibility_to_string(Visibility v) {
        return (v == Visibility::PUBLIC) ? "public" : "private";
    }

    std::string type_to_string(const Type& t) {
        switch (t.type) {
            case TypeCategory::STRING: return "string";
            case TypeCategory::INT: 
                if (t.bit_width == 32 && !t.is_signed) return "uint";
                if (t.bit_width == 32 && t.is_signed) return "int";
                // for explicit sized types, return their actual name
                if (!t.name.empty()) return t.name;
                // Fallback for unspecified int types
                return t.is_signed ? "int" : "uint";
            case TypeCategory::FLOAT:
                if (t.bit_width == 32) return "f32";
                if (t.bit_width == 64) return "f64";
                if (!t.name.empty()) return t.name;
                return "float";
            case TypeCategory::BOOL: return "bool";
            case TypeCategory::CHAR: return "char";
            case TypeCategory::CLASS: return "class";
            case TypeCategory::CRATE: return "crate";
            case TypeCategory::VOID: return "void";
            case TypeCategory::PAIR: return "pair";
            default: return "unknown";
        }
    }
};
