#pragma once


class BinaryExpression;

class Visitor {
public:

    virtual void visit(BinaryExpression& b);

};
