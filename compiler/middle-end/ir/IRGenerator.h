#pragma once

#include "TAC.h"

#include "../../ast/Node.h"

#include <optional>

namespace ir {

class IRGenerator final : public Visitor {
public:
    IRGenerator() = default;

    ModuleIR generate(GlobalNode& root);

    void visit(GlobalNode& global) override;
    void visit(VariableNode& v) override;
    void visit(BinaryExpression& b) override;
    void visit(FnNode& fn) override;
    void visit(FnCallNode& call) override;
    void visit(ConstructorNode& c) override;
    void visit(BodyNode& b) override;
    void visit(ClassNode& c) override;
    void visit(CrateNode& cr) override;
    void visit(WhileNode& w) override;
    void visit(ForNode& f) override;
    void visit(IfNode& i) override;
    void visit(ElseIfNode& i) override;
    void visit(ElseNode& e) override;
    void visit(MatchNode& m) override;
    void visit(UnaryIncrNode& u) override;
    void visit(DotNode& d) override;
    void visit(CascadeNode& c) override;
    void visit(PipelineNode& p) override;
    void visit(ReturnNode& r) override;
    void visit(YieldNode& y) override;
    void visit(ConversionNode& c) override;
    void visit(IdentifierCondition& c) override;
    void visit(IntegerCondition& c) override;
    void visit(DoubleCondition& c) override;
    void visit(BoolCondition& c) override;
    void visit(StringCondition& c) override;
    void visit(CharCondition& c) override;

private:
    ModuleIR module_;
    FunctionBuilder* current_builder_ = nullptr;
    std::optional<Operand> last_value_;
    Temp emit_temp();
    Operand take_value();
    void set_value(Operand operand);
    void clear_values();

    Operand lower_condition(Condition& condition);
    Operand lower_expression(Condition& condition);
    void lower_body(BodyNode& body);
    std::optional<std::string> lower_symbol(Node& node);
};

} // namespace ir
