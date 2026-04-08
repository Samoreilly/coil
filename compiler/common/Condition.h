#pragma once

#include "../Token.h"
#include <memory>
#include <string>

#include "Visitor.h"



class Node {
public:

    virtual ~Node() = default;
    virtual void accept(Visitor& v) = 0;
    virtual void print() const = 0;
};

class Condition : public Node {
public:

    virtual ~Condition() = default;
    virtual void accept(Visitor& v) = 0;
    virtual void print() const = 0;
};


class BinaryExpression : public Condition {
public:

    std::unique_ptr<Condition> left;
    std::string op;
    std::unique_ptr<Condition> right;
    
    void accept(Visitor& v) override {
        v.visit(*this);
    }

    void print() const override;

};

class IdentifierCondition : public Condition {
public:

    Token token;

    IdentifierCondition(Token t) : token(t) {}

    void accept(Visitor& v) override {
        v.visit(*this);
    }

    void print() const override;
};

class ReturnNode : public Condition {
public:

    std::unique_ptr<Condition> ret;


    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};

class IntegerCondition : public Condition {
public:


    Token token;

    IntegerCondition(Token t) : token(t) {}

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};

class DoubleCondition : public Condition {
public:


    Token token;

    DoubleCondition(Token t) : token(t) {}

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};

class CharCondition : public Condition {
public:


    Token token;

    CharCondition(Token t) : token(t) {}

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};

class BoolCondition : public Condition {
public:


    Token token;

    BoolCondition(Token t) : token(t) {}

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};

class StringCondition : public Condition {
public:

    Token token;

    StringCondition(Token t) : token(t) {}

    void accept(Visitor& v) override {
        v.visit(*this);
    }
   
    void print() const override;
};


