#pragma once

#include "Condition.h"
#include "Node.h"
#include "../Token.h"
#include <vector>
#include <optional>
#include <variant>

class BodyNode;



//underlying type is for objects actual type rather than there name
struct Parameter {
    std::string name;
    std::variant<TYPE, std::string> type; 
};


class GlobalNode : public Node {
public:

    std::vector<std::unique_ptr<Node>> globals;
    
    void accept(Visitor& v) override {
        v.visit(*this);
    }

    void print() const override;

};

class VariableNode : public Node {
public:
    
    std::optional<TYPE> type;
    std::string name;
    std::optional<std::unique_ptr<Condition>> init;

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;

};

class FnNode : public Node {
public:

    Visibility vis = Visibility::PUBLIC;
    std::optional<TYPE> return_type;
    std::string name;
    std::vector<std::unique_ptr<Parameter>> parameters;
    
    std::unique_ptr<BodyNode> body;
    
    void accept(Visitor& v) override {
        v.visit(*this);
    }
    void print() const override;

};

class FnCallNode : public Condition {
public:
    
    std::string name;
    std::vector<std::unique_ptr<Condition>> arguments;
    
    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};

class BodyNode : public Node {
public:

    std::vector<std::unique_ptr<Node>> statements;

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;

};

class CrateNode : public Node {
public:
    
    Visibility vis = Visibility::PUBLIC;
    std::string name;
    std::vector<std::unique_ptr<VariableNode>> crate_vars;

    void accept(Visitor& v) override {
        v.visit(*this);
    }

    void print() const override;
};

class DotNode : public Condition {
public:

    std::unique_ptr<Condition> left;
    std::unique_ptr<Condition> right;

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;

};

class UnaryIncrNode : public Node {
public:

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};

class ForNode : public Node {
public:

    std::optional<std::unique_ptr<VariableNode>> init;
    std::optional<std::unique_ptr<Condition>> cond;
    std::optional<std::unique_ptr<Node>> incr;
    
    std::unique_ptr<BodyNode> for_body;

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};

class WhileNode : public Node {
public:

    std::optional<std::unique_ptr<Condition>> cond;
    std::unique_ptr<BodyNode> while_body;

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};

class IfNode : public Node {
public:

    std::unique_ptr<Condition> cond;
    std::unique_ptr<BodyNode> if_body;

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};

class ElseIfNode : public Node {
public:

    std::unique_ptr<Condition> cond;
    std::unique_ptr<BodyNode> elif_body;

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};

class ElseNode : public Node {
public:

    std::unique_ptr<BodyNode> else_body;

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;

};


class MatchNode : public Node {
public:

    std::unique_ptr<Condition> input;
    std::vector<std::unique_ptr<Condition>> cases;

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};

class PipelineNode : public Condition {
public:
    
    std::unique_ptr<Condition> left;
    std::unique_ptr<Condition> right;

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};

class CascadeNode : public Node {
public:

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};







