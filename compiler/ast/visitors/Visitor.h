#pragma once

#include <memory>

class GlobalNode;


class VariableNode;
class BinaryExpression;
class FnNode;
class FnCallNode;
class ConstructorNode;
class BodyNode;
class ClassNode;
class CrateNode;
class WhileNode;
class ForNode;
class IfNode;
class ElseIfNode;
class ElseNode;
class MatchNode;
class UnaryIncrNode;
class DotNode;
class CascadeNode;
class PipelineNode;
class ReturnNode;
class ConversionNode;
class IdentifierCondition;
class IntegerCondition;
class DoubleCondition;
class BoolCondition;
class StringCondition;
class CharCondition;



class Visitor {
public:

    virtual void visit(GlobalNode& global) = 0;

    virtual void visit(VariableNode& v) = 0;
    virtual void visit(BinaryExpression& b) = 0;
    virtual void visit(FnNode& b) = 0;
    virtual void visit(FnCallNode& b) = 0;
    virtual void visit(ConstructorNode& c) = 0;
    virtual void visit(BodyNode& b) = 0;
    virtual void visit(ClassNode& c) = 0;
    virtual void visit(CrateNode& cr) = 0;
    virtual void visit(WhileNode& b) = 0;
    virtual void visit(ForNode& b) = 0;
    
    virtual void visit(IfNode& b) = 0;
    virtual void visit(ElseIfNode& b) = 0;
    virtual void visit(ElseNode& b) = 0;
    virtual void visit(MatchNode& b) = 0;
    virtual void visit(UnaryIncrNode& b) = 0;
    virtual void visit(DotNode& b) = 0;
    

    virtual void visit(CascadeNode& id) = 0;
    virtual void visit(PipelineNode& id) = 0;
    virtual void visit(ReturnNode& b) = 0;
    virtual void visit(ConversionNode& b) = 0;

    virtual void visit(IdentifierCondition& b) = 0;
    virtual void visit(IntegerCondition& id) = 0;
    virtual void visit(DoubleCondition& id) = 0;
    virtual void visit(BoolCondition& id) = 0;
    virtual void visit(StringCondition& id) = 0;
    virtual void visit(CharCondition& id) = 0;

    
};
