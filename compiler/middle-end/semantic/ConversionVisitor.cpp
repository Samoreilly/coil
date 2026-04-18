#include "ConversionVisitor.h"

#include <memory>

namespace {

std::unique_ptr<Condition> convert_if_needed(std::unique_ptr<Condition> expr, const Type& from, const Type& to) {
    if (!expr || same_type(from, to) || !can_implicitly_convert(from, to)) {
        return expr;
    }

    return std::make_unique<ConversionNode>(std::move(expr), from, to, ConversionKind::IMPLICIT);
}

}

void ConversionVisitor::convert_function_arguments(FnCallNode& call) {
    if (!table) {
        return;
    }

    auto* entry = table->lookup_global(call.name);
    if (!entry) {
        return;
    }

    if (entry->param_types.size() != call.arguments.size()) {
        return;
    }

    for (std::size_t i = 0; i < call.arguments.size(); ++i) {
        auto& arg = call.arguments[i];
        if (!arg) {
            continue;
        }

        auto actual = condition_to_type(*arg, table);
        if (!actual) {
            continue;
        }

        arg = convert_if_needed(std::move(arg), *actual, entry->param_types[i]);
    }
}

void ConversionVisitor::convert_binary(BinaryExpression& b) {
    if (!b.left || !b.right) {
        return;
    }

    auto lhs = condition_to_type(*b.left, table);
    auto rhs = condition_to_type(*b.right, table);
    if (!lhs || !rhs) {
        return;
    }

    if (is_arithmetic_operator(b.op) || is_comparison_operator(b.op)) {
        auto promoted = common_numeric_type(*lhs, *rhs);
        if (!promoted) {
            return;
        }

        b.left = convert_if_needed(std::move(b.left), *lhs, *promoted);
        b.right = convert_if_needed(std::move(b.right), *rhs, *promoted);
    }
}

void ConversionVisitor::visit(GlobalNode& global) {
    for (auto& node : global.globals) {
        if (node) {
            node->accept(*this);
        }
    }
}

void ConversionVisitor::visit(VariableNode& v) {
    if (!v.init || !*v.init || !v.type) {
        return;
    }

    auto actual = condition_to_type(*v.init.value(), table);
    if (!actual) {
        return;
    }

    *v.init = convert_if_needed(std::move(*v.init), *actual, *v.type);
}

void ConversionVisitor::visit(BinaryExpression& b) {
    convert_binary(b);
}

void ConversionVisitor::visit(FnNode& fn) {
    auto* saved = table;
    if (table) {
        if (auto* entry = table->lookup_global(fn.name); entry && entry->scope) {
            table = entry->scope.get();
        }
    }

    if (fn.body) {
        visit(*fn.body, table);
    }

    table = saved;
}

void ConversionVisitor::visit(FnCallNode& b) {
    convert_function_arguments(b);
}

void ConversionVisitor::visit(ConstructorNode& c) {
    auto* saved = table;
    if (table) {
        if (auto* entry = table->lookup_global(c.name); entry && entry->scope) {
            table = entry->scope.get();
        }
    }

    if (c.body) {
        visit(*c.body, table);
    }

    table = saved;
}

void ConversionVisitor::visit(BodyNode& b) {
    visit(b, table);
}

void ConversionVisitor::visit(BodyNode& b, ::Semantic::SymbolTable* current_table) {
    auto* saved = table;
    table = current_table ? current_table : table;

    for (auto& stmt : b.statements) {
        if (stmt) {
            stmt->accept(*this);
        }
    }

    table = saved;
}

void ConversionVisitor::visit(ClassNode& c) {
    auto* saved = table;
    if (table) {
        if (auto* entry = table->lookup_global(c.name); entry && entry->scope) {
            table = entry->scope.get();
        }
    }

    if (c.body) {
        visit(*c.body, table);
    }

    table = saved;
}

void ConversionVisitor::visit(CrateNode& cr) {
    (void)cr;
}

void ConversionVisitor::visit(WhileNode& whl) {
    if (whl.cond && *whl.cond) {
        auto actual = condition_to_type(*whl.cond.value(), table);
        if (actual) {
            *whl.cond = convert_if_needed(std::move(*whl.cond), *actual, TYPES.at("bool"));
        }
    }

    if (whl.body) {
        visit(*whl.body, table);
    }
}

void ConversionVisitor::visit(ForNode& for_node) {
    if (for_node.init && *for_node.init) {
        for_node.init.value()->accept(*this);
    }

    if (for_node.cond && *for_node.cond) {
        auto actual = condition_to_type(*for_node.cond.value(), table);
        if (actual) {
            *for_node.cond = convert_if_needed(std::move(*for_node.cond), *actual, TYPES.at("bool"));
        }
    }

    if (for_node.incr && *for_node.incr) {
        for_node.incr.value()->accept(*this);
    }

    if (for_node.for_body) {
        visit(*for_node.for_body, table);
    }
}

void ConversionVisitor::visit(IfNode& if_node) {
    if (if_node.cond) {
        auto actual = condition_to_type(*if_node.cond, table);
        if (actual) {
            if_node.cond = convert_if_needed(std::move(if_node.cond), *actual, TYPES.at("bool"));
        }
    }

    if (if_node.body) {
        visit(*if_node.body, table);
    }

    for (auto& else_if : if_node.else_ifs) {
        if (else_if) {
            else_if->accept(*this);
        }
    }

    if (if_node.else_node) {
        if_node.else_node->accept(*this);
    }
}

void ConversionVisitor::visit(ElseIfNode& else_if_node) {
    if (else_if_node.cond) {
        auto actual = condition_to_type(*else_if_node.cond, table);
        if (actual) {
            else_if_node.cond = convert_if_needed(std::move(else_if_node.cond), *actual, TYPES.at("bool"));
        }
    }

    if (else_if_node.body) {
        visit(*else_if_node.body, table);
    }
}

void ConversionVisitor::visit(ElseNode& else_node) {
    if (else_node.body) {
        visit(*else_node.body, table);
    }
}

void ConversionVisitor::visit(CascadeNode& id) {
    (void)id;
}

void ConversionVisitor::visit(ConversionNode& b) {
    if (b.expr) {
        b.expr->accept(*this);
    }
}

void ConversionVisitor::visit(MatchNode& m) {
    if (m.input) {
        m.input->accept(*this);
    }

    for (auto& case_item : m.cases) {
        if (case_item.pattern) {
            case_item.pattern->accept(*this);
        }
        if (case_item.body) {
            auto case_scope = std::make_shared<::Semantic::SymbolTable>("match_case");
            case_scope->parent = table;
            visit(*case_item.body, case_scope.get());
        }
    }
}
void ConversionVisitor::visit(UnaryIncrNode&) {}
void ConversionVisitor::visit(DotNode&) {}
void ConversionVisitor::visit(PipelineNode&) {}
void ConversionVisitor::visit(ReturnNode& b) {
    if (b.ret) {
        b.ret->accept(*this);
    }
}
void ConversionVisitor::visit(YieldNode& b) {
    if (b.value) {
        b.value->accept(*this);
    }
}
void ConversionVisitor::visit(IdentifierCondition&) {}
void ConversionVisitor::visit(IntegerCondition&) {}
void ConversionVisitor::visit(DoubleCondition&) {}
void ConversionVisitor::visit(BoolCondition&) {}
void ConversionVisitor::visit(StringCondition&) {}
void ConversionVisitor::visit(CharCondition&) {}
