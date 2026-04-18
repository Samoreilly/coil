#include "IRGenerator.h"

#include <utility>

namespace ir {

namespace {

Opcode compound_opcode(const std::string& op) {
    if (op == "+=" || op == "++") return Opcode::Add;
    if (op == "-=" || op == "--") return Opcode::Sub;
    if (op == "*=") return Opcode::Mul;
    if (op == "/=") return Opcode::Div;
    return Opcode::Nop;
}

bool is_compound_update(const std::string& op) {
    return op == "+=" || op == "-=" || op == "*=" || op == "/=";
}

Operand as_bool_operand(const std::optional<Operand>& value) {
    return value ? *value : Operand{boolean(true)};
}

void emit_block_jump(ir::FunctionBuilder& builder, const ir::Label& label) {
    builder.emit_jump(label);
}

} // namespace

Temp IRGenerator::emit_temp() {
    return current_builder_->new_temp();
}

Operand IRGenerator::take_value() {
    if (!last_value_) {
        return {};
    }

    auto value = std::move(*last_value_);
    last_value_.reset();
    return value;
}

void IRGenerator::set_value(Operand operand) {
    last_value_ = std::move(operand);
}

void IRGenerator::clear_values() {
    last_value_.reset();
}

Operand IRGenerator::lower_condition(Condition& condition) {
    condition.accept(*this);
    return take_value();
}

Operand IRGenerator::lower_expression(Condition& condition) {
    condition.accept(*this);
    return take_value();
}

void IRGenerator::lower_body(BodyNode& body) {
    for (auto& stmt : body.statements) {
        if (stmt) {
            stmt->accept(*this);
            clear_values();
        }
    }
}

std::optional<std::string> IRGenerator::lower_symbol(Node& node) {
    if (auto* ident = dynamic_cast<IdentifierCondition*>(&node)) {
        return ident->token.token_value;
    }

    if (auto* dot = dynamic_cast<DotNode*>(&node)) {
        if (dot->right) {
            if (auto* ident = dynamic_cast<IdentifierCondition*>(dot->right.get())) {
                return ident->token.token_value;
            }
        }
    }

    return std::nullopt;
}

ModuleIR IRGenerator::generate(GlobalNode& root) {
    module_.functions.clear();
    root.accept(*this);
    return module_;
}

void IRGenerator::visit(GlobalNode& global) {
    for (auto& node : global.globals) {
        if (node) {
            node->accept(*this);
        }
    }
}

void IRGenerator::visit(VariableNode& v) {
    if (!current_builder_ || !v.name) {
        clear_values();
        return;
    }

    auto destination = lower_symbol(*v.name);
    if (!destination) {
        clear_values();
        return;
    }

    if (v.op && *v.op == "=") {
        if (v.init && *v.init) {
            auto value = lower_condition(*v.init.value());
            if (!is_empty(value)) {
                current_builder_->emit_assign(named(*destination), std::move(value));
            }
        }
    } else if (v.op && is_compound_update(*v.op)) {
        if (v.init && *v.init) {
            auto rhs = lower_condition(*v.init.value());
            if (!is_empty(rhs)) {
                auto temp_value = emit_temp();
                current_builder_->emit_binary(compound_opcode(*v.op), temp(temp_value.id), named(*destination), std::move(rhs));
                current_builder_->emit_assign(named(*destination), temp(temp_value.id));
            }
        }
    } else if (v.init && *v.init) {
        auto value = lower_condition(*v.init.value());
        if (!is_empty(value)) {
            current_builder_->emit_assign(named(*destination), std::move(value));
        }
    }
    clear_values();
}

void IRGenerator::visit(BinaryExpression& b) {
    auto lhs = b.left ? lower_condition(*b.left) : Operand{};
    auto rhs = b.right ? lower_condition(*b.right) : Operand{};
    auto temp_value = emit_temp();

    if (b.op == "+") {
        current_builder_->emit_binary(Opcode::Add, temp(temp_value.id), std::move(lhs), std::move(rhs));
    } else if (b.op == "-") {
        current_builder_->emit_binary(Opcode::Sub, temp(temp_value.id), std::move(lhs), std::move(rhs));
    } else if (b.op == "*") {
        current_builder_->emit_binary(Opcode::Mul, temp(temp_value.id), std::move(lhs), std::move(rhs));
    } else if (b.op == "/") {
        current_builder_->emit_binary(Opcode::Div, temp(temp_value.id), std::move(lhs), std::move(rhs));
    } else if (b.op == "<") {
        current_builder_->emit_binary(Opcode::CompareLt, temp(temp_value.id), std::move(lhs), std::move(rhs));
    } else if (b.op == ">") {
        current_builder_->emit_binary(Opcode::CompareGt, temp(temp_value.id), std::move(lhs), std::move(rhs));
    } else if (b.op == "<=") {
        current_builder_->emit_binary(Opcode::CompareLe, temp(temp_value.id), std::move(lhs), std::move(rhs));
    } else if (b.op == ">=") {
        current_builder_->emit_binary(Opcode::CompareGe, temp(temp_value.id), std::move(lhs), std::move(rhs));
    } else if (b.op == "==") {
        current_builder_->emit_binary(Opcode::CompareEq, temp(temp_value.id), std::move(lhs), std::move(rhs));
    } else if (b.op == "!=") {
        current_builder_->emit_binary(Opcode::CompareNe, temp(temp_value.id), std::move(lhs), std::move(rhs));
    }

    set_value(temp(temp_value.id));
}

void IRGenerator::visit(FnNode& fn) {
    FunctionBuilder builder(fn.name);
    auto* saved = current_builder_;
    current_builder_ = &builder;
    current_builder_->emit_label(current_builder_->new_label("entry"));
    if (fn.body) {
        fn.body->accept(*this);
    }
    module_.functions.push_back(builder.finish());
    current_builder_ = saved;
    clear_values();
}

void IRGenerator::visit(FnCallNode& call) {
    if (!current_builder_) {
        clear_values();
        return;
    }

    for (auto& arg : call.arguments) {
        if (arg) {
            current_builder_->emit_param(lower_expression(*arg));
        }
    }

    auto dest = emit_temp();
    current_builder_->emit_call(temp(dest.id), named(call.name), call.arguments.size());
    set_value(temp(dest.id));
}

void IRGenerator::visit(ConstructorNode& c) {
    if (c.body) {
        c.body->accept(*this);
    }
    clear_values();
}

void IRGenerator::visit(BodyNode& b) {
    lower_body(b);
}

void IRGenerator::visit(ClassNode& c) {
    if (c.body) {
        c.body->accept(*this);
    }
    clear_values();
}

void IRGenerator::visit(CrateNode& cr) {
    for (auto& var : cr.crate_vars) {
        if (var) {
            var->accept(*this);
        }
    }
    clear_values();
}

void IRGenerator::visit(WhileNode& w) {
    if (!current_builder_) {
        clear_values();
        return;
    }

    auto start = current_builder_->new_label("while_start");
    auto body = current_builder_->new_label("while_body");
    auto end = current_builder_->new_label("while_end");

    current_builder_->emit_label(start);
    auto cond = w.cond && *w.cond ? lower_condition(*w.cond.value()) : Operand{boolean(true)};
    current_builder_->emit_jump_if_false(std::move(cond), end);
    emit_block_jump(*current_builder_, body);

    current_builder_->emit_label(body);

    if (w.body) {
        w.body->accept(*this);
    }
    current_builder_->emit_jump(start);
    current_builder_->emit_label(end);
    clear_values();
}

void IRGenerator::visit(ForNode& f) {
    if (!current_builder_) {
        clear_values();
        return;
    }

    auto start = current_builder_->new_label("for_start");
    auto body = current_builder_->new_label("for_body");
    auto incr = current_builder_->new_label("for_incr");
    auto end = current_builder_->new_label("for_end");

    if (f.init && *f.init) {
        (*f.init)->accept(*this);
    }

    current_builder_->emit_label(start);
    auto cond = f.cond && *f.cond ? lower_condition(*f.cond.value()) : Operand{boolean(true)};
    current_builder_->emit_jump_if_false(std::move(cond), end);
    emit_block_jump(*current_builder_, body);

    current_builder_->emit_label(body);

    if (f.for_body) {
        f.for_body->accept(*this);
    }
    current_builder_->emit_jump(incr);

    current_builder_->emit_label(incr);
    if (f.incr && *f.incr) {
        (*f.incr)->accept(*this);
    }
    current_builder_->emit_jump(start);
    current_builder_->emit_label(end);
    clear_values();
}

void IRGenerator::visit(IfNode& i) {
    if (!current_builder_) {
        clear_values();
        return;
    }

    auto end = current_builder_->new_label("if_end");
    auto then_label = current_builder_->new_label("if_then");
    std::vector<Label> branch_labels;
    branch_labels.reserve(i.else_ifs.size() + (i.else_node ? 1 : 0));

    for (std::size_t idx = 0; idx < i.else_ifs.size(); ++idx) {
        branch_labels.push_back(current_builder_->new_label("if_branch"));
    }
    if (i.else_node) {
        branch_labels.push_back(current_builder_->new_label("if_else"));
    }

    const auto first_false = branch_labels.empty() ? end : branch_labels.front();

    auto cond = i.cond ? lower_condition(*i.cond) : Operand{boolean(true)};
    current_builder_->emit_jump_if_false(std::move(cond), first_false);
    emit_block_jump(*current_builder_, then_label);

    current_builder_->emit_label(then_label);

    if (i.body) {
        i.body->accept(*this);
    }
    current_builder_->emit_jump(end);

    for (std::size_t idx = 0; idx < i.else_ifs.size(); ++idx) {
        current_builder_->emit_label(branch_labels[idx]);
        const auto next_false = (idx + 1 < i.else_ifs.size())
            ? branch_labels[idx + 1]
            : (i.else_node ? branch_labels.back() : end);

        auto else_if_cond = i.else_ifs[idx] && i.else_ifs[idx]->cond ? lower_condition(*i.else_ifs[idx]->cond) : Operand{boolean(true)};
        current_builder_->emit_jump_if_false(std::move(else_if_cond), next_false);

        if (i.else_ifs[idx] && i.else_ifs[idx]->body) {
            i.else_ifs[idx]->body->accept(*this);
        }

        current_builder_->emit_jump(end);
    }

    if (i.else_node && !branch_labels.empty()) {
        current_builder_->emit_label(branch_labels.back());
        if (i.else_node->body) {
            i.else_node->body->accept(*this);
        }
    }

    current_builder_->emit_label(end);
    clear_values();
}

void IRGenerator::visit(ElseIfNode& i) {
    clear_values();
}

void IRGenerator::visit(ElseNode& e) {
    clear_values();
}

void IRGenerator::visit(MatchNode& m) {
    if (!current_builder_) {
        clear_values();
        return;
    }

    auto matched = m.input ? lower_condition(*m.input) : Operand{boolean(true)};
    auto end = current_builder_->new_label("match_end");

    std::vector<Label> labels;
    labels.reserve(m.cases.size());
    for (std::size_t i = 0; i < m.cases.size(); ++i) {
        labels.push_back(current_builder_->new_label("match_case"));
    }

    for (std::size_t i = 0; i < m.cases.size(); ++i) {
        current_builder_->emit_label(labels[i]);
        const auto next = (i + 1 < m.cases.size()) ? labels[i + 1] : end;

        auto pattern = m.cases[i].pattern ? lower_condition(*m.cases[i].pattern) : Operand{boolean(true)};
        auto cmp = emit_temp();
        current_builder_->emit_binary(Opcode::CompareEq, temp(cmp.id), matched, std::move(pattern));
        current_builder_->emit_jump_if_false(temp(cmp.id), next);

        if (m.cases[i].body) {
            m.cases[i].body->accept(*this);
        }

        current_builder_->emit_jump(end);
    }

    current_builder_->emit_label(end);
    clear_values();
}

void IRGenerator::visit(UnaryIncrNode& u) {
    if (!current_builder_ || !u.name) {
        clear_values();
        return;
    }

    auto destination = lower_symbol(*u.name);
    if (!destination) {
        clear_values();
        return;
    }

    auto one = integer(1);
    auto temp_value = emit_temp();

    if (u.op == "++") {
        current_builder_->emit_binary(Opcode::Add, temp(temp_value.id), named(*destination), one);
    } else if (u.op == "--") {
        current_builder_->emit_binary(Opcode::Sub, temp(temp_value.id), named(*destination), one);
    }

    current_builder_->emit_assign(named(*destination), temp(temp_value.id));
    clear_values();
}

void IRGenerator::visit(DotNode&) { clear_values(); }
void IRGenerator::visit(CascadeNode&) { clear_values(); }
void IRGenerator::visit(PipelineNode&) { clear_values(); }
void IRGenerator::visit(ReturnNode& r) {
    if (r.ret) {
        auto value = lower_condition(*r.ret);
        if (current_builder_) current_builder_->emit_return(std::move(value));
    } else if (current_builder_) {
        current_builder_->emit_return();
    }
    clear_values();
}
void IRGenerator::visit(YieldNode&) { clear_values(); }
void IRGenerator::visit(ConversionNode&) { clear_values(); }
void IRGenerator::visit(IdentifierCondition& c) { set_value(named(c.token.token_value)); }
void IRGenerator::visit(IntegerCondition& c) { set_value(integer(std::stoll(c.token.token_value))); }
void IRGenerator::visit(DoubleCondition& c) { set_value(floating(std::stod(c.token.token_value))); }
void IRGenerator::visit(BoolCondition& c) { set_value(boolean(c.token.token_value == "true")); }
void IRGenerator::visit(StringCondition& c) { set_value(string_literal(c.token.token_value)); }
void IRGenerator::visit(CharCondition& c) { set_value(character(c.token.token_value.empty() ? '\0' : c.token.token_value.front())); }

} // namespace ir
