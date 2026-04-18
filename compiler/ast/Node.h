#pragma once

#include "Condition.h"
#include <vector>
#include <optional>
#include <variant>

namespace Semantic {
    struct SymbolTable;
}

class BodyNode;



//underlying type is for objects actual type rather than there name
struct Parameter {
    std::variant<Type, std::string> type;
    std::string name;

    Parameter(Type t) : type(t) {}
    Parameter(std::string t) : type(std::move(t)) {}

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
    
    Visibility vis = Visibility::PUBLIC;
    ACCESS access = ACCESS::MUTABLE;

    std::optional<Type> type;
    bool inferred_type = false;
    std::unique_ptr<Condition> name;
    std::optional<std::unique_ptr<Condition>> init;
    std::optional<std::string> op;//+, -, *=, -=

    VariableNode() {}

    VariableNode(std::unique_ptr<Condition> n,
                 Visibility v = Visibility::PUBLIC,
                 std::optional<Type> t = std::nullopt,
                 std::optional<std::unique_ptr<Condition>> i = std::nullopt,
                 std::optional<std::string> o = std::nullopt)
    : vis(v), type(t), name(std::move(n)), init(std::move(i)), op(o) {}
   
    void accept(Visitor& v) override {
            v.visit(*this);
    }
   
    void print() const override;

};

class FnNode : public Node {
public:

    Visibility vis = Visibility::PUBLIC;
    std::optional<Type> return_type;
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

class ConstructorNode : public Condition {
public:

    Visibility vis = Visibility::PUBLIC;
    std::string name;
    std::vector<std::unique_ptr<Parameter>> params;
    std::unique_ptr<BodyNode> body;

   void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};

class BodyNode : public Node {
public:

    std::vector<std::unique_ptr<Node>> statements;
    std::shared_ptr<Semantic::SymbolTable> scope;

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;

};

class ClassNode : public Node {
public:

    Visibility vis = Visibility::PUBLIC;
    std::string name;
    std::unique_ptr<BodyNode> body;

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

    std::unique_ptr<Node> left; //access e.g. object.earth.ire
    std::unique_ptr<Node> right;//county - will change this to a string later 

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;

};

class UnaryIncrNode : public Node {
public:

    std::unique_ptr<Condition> name;
    std::string op;

    UnaryIncrNode(std::unique_ptr<Condition> n, std::string o) : name(std::move(n)), op(std::move(o)) {}

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
    std::unique_ptr<BodyNode> body;

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};

class IfNode : public Node {
public:

    std::unique_ptr<Condition> cond;
    std::unique_ptr<BodyNode> body;
    std::vector<std::unique_ptr<ElseIfNode>> else_ifs;
    std::unique_ptr<ElseNode> else_node;

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};

class ElseIfNode : public Node {
public:

    std::unique_ptr<Condition> cond;
    std::unique_ptr<BodyNode> body;

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};

class ElseNode : public Node {
public:

    std::unique_ptr<BodyNode> body;

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;

};


class MatchNode : public Condition {
public:

    struct MatchCase {
        std::unique_ptr<Condition> pattern;
        std::unique_ptr<BodyNode> body;
    };

    std::unique_ptr<Condition> input;
    std::vector<MatchCase> cases;

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


class CascadeStep {
public:
        
    //..num = 10

    std::string var_name;
    std::string op;
    std::unique_ptr<Condition> condition;
    
};

class CascadeNode : public Condition {
public:

    std::unique_ptr<Condition> name;//objects name
    std::vector<std::unique_ptr<CascadeStep>> cascades;

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};
