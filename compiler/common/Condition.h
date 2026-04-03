#pragma once

#include <memory>
#include <string>

#include "Visitor.h"



class Node {
public:

    virtual ~Node() = default;
    virtual void accept(Visitor& v) = 0;

};

class Condition : public Node {
public:

    virtual ~Condition() = default;
    virtual void accept(Visitor& v);
};


class BinaryExpression : public Condition {
public:

    std::unique_ptr<Condition> left;
    std::string op;
    std::unique_ptr<Condition> right;

};
