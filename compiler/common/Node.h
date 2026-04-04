#pragma once

#include "Condition.h"
#include "Node.h"
#include "../Token.h"
#include <vector>
#include <optional>

class BodyNode;


class VariableNode : public Node {
public:
    
    std::optional<Type> type;
    std::string name;
    std::optional<std::unique_ptr<Condition>> init;


    void print() const override;

};

class DutyNode : public Node {
public:

    Visibility vis = Visibility::PUBLIC;
    std::unique_ptr<Condition> return_type;
    std::vector<std::unique_ptr<VariableNode>> parameters;
    
    std::unique_ptr<BodyNode> duty_body;
  
    void print() const override;

};

class DutyCallNode : public Condition {
public:
    
    std::string duty_name;
    std::vector<std::unique_ptr<Condition>> arguments;
    

    void print() const override;
};

class BodyNode : public Node {
public:

    std::vector<std::unique_ptr<Node>> statements;

    void print() const override;

};

class CrateNode : public Node {
public:
    
    Visibility vis = Visibility::PUBLIC;
    std::vector<std::unique_ptr<VariableNode>> crate_vars;

    void print() const override;
};

class DotNode : public Node  {
public:


    void print() const override;

};

class UnaryIncrNode : public Node {
public:

    void print() const override;
};

class ForNode : public Node {
public:

    std::optional<std::unique_ptr<VariableNode>> init;
    std::optional<std::unique_ptr<Condition>> cond;
    std::optional<std::unique_ptr<Node>> incr;
    
    std::unique_ptr<BodyNode> for_body;

    void print() const override;
};

class WhileNode : public Node {
public:

    std::optional<std::unique_ptr<Condition>> cond;
    std::unique_ptr<BodyNode> while_body;

    void print() const override;
};

class IfNode : public Node {
public:

    std::unique_ptr<Condition> cond;
    std::unique_ptr<BodyNode> if_body;

    void print() const override;
};

class ElseIfNode : public Node {
public:

    std::unique_ptr<Condition> cond;
    std::unique_ptr<BodyNode> elif_body;

    void print() const override;
};

class ElseNode : public Node {
public:

    std::unique_ptr<BodyNode> else_body;

    void print() const override;

};


class MatchNode : public Node {
public:

    std::unique_ptr<Condition> input;
    std::vector<std::unique_ptr<Condition>> cases;

    void print() const override;
};

class PipelineNode : public Node {
public:

    void print() const override;
};

class CascadeNode : public Node {
public:


    void print() const override;
};







