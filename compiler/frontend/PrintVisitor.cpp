#include "PrintVisitor.h"
#include "../common/Node.h"
#include "../common/Condition.h"

void PrintVisitor::visit(GlobalNode& global) {
    print_indent();
    out << "GlobalNode\n";
    indent_level++;
    for (auto& node : global.globals) {
        if (node) node->accept(*this);
    }
    indent_level--;
}

void PrintVisitor::visit(ClassNode& c) {
    print_indent();
    out << "ClassNode (name: " << c.name << ", vis: " << visibility_to_string(c.vis) << ")\n";
    indent_level++;
    if (c.body) {
        c.body->accept(*this);
    }
    indent_level--;
}

void PrintVisitor::visit(VariableNode& v) {
    print_indent();
    out << "VariableNode (vis: " << visibility_to_string(v.vis) 
        << ", access: " << (v.access == ACCESS::IMMUTABLE ? "immut" : "mut") << ")\n";
    indent_level++;
    if (v.type) {
        print_indent();
        out << "Type: " << type_to_string(*v.type) << "\n";
    }
    if (v.name) {
        print_indent();
        out << "Name:\n";
        indent_level++;
        v.name->accept(*this);
        indent_level--;
    }
    if (v.init && *v.init) {
        print_indent();
        out << "Init:\n";
        indent_level++;
        (*v.init)->accept(*this);
        indent_level--;
    }
    indent_level--;
}

void PrintVisitor::visit(BinaryExpression& b) {
    print_indent();
    out << "BinaryExpression (" << b.op << ")\n";
    indent_level++;
    if (b.left) b.left->accept(*this);
    if (b.right) b.right->accept(*this);
    indent_level--;
}

void PrintVisitor::visit(FnNode& f) {
    print_indent();
    out << "FnNode (name: " << f.name << ", vis: " << visibility_to_string(f.vis) << ")\n";
    indent_level++;
    if (f.return_type) {
        print_indent();
        out << "Returns: " << type_to_string(*f.return_type) << "\n";
    }
    print_indent();
    out << "Parameters:\n";
    indent_level++;
    for (auto& param : f.parameters) {
        print_indent();
        out << param->name << " : ";
        if (std::holds_alternative<TYPE>(param->type)) {
            out << type_to_string(std::get<TYPE>(param->type));
        } else {
            out << std::get<std::string>(param->type);
        }
        out << "\n";
    }
    indent_level--;
    if (f.body) f.body->accept(*this);
    indent_level--;
}

void PrintVisitor::visit(FnCallNode& f) {
    print_indent();
    out << "FnCallNode (name: " << f.name << ")\n";
    indent_level++;
    for (auto& arg : f.arguments) {
        if (arg) arg->accept(*this);
    }
    indent_level--;
}

void PrintVisitor::visit(ConstructorNode& c) {
    print_indent();
    out << "ConstructorNode (name: " << c.name << ", vis: " << visibility_to_string(c.vis) << ")\n";
    indent_level++;
    for (auto& param : c.params) {
        if (param) param->accept(*this);
    }
    if (c.body) c.body->accept(*this);
    indent_level--;
}

void PrintVisitor::visit(BodyNode& b) {
    print_indent();
    out << "BodyNode\n";
    indent_level++;
    for (auto& stmt : b.statements) {
        if (stmt) stmt->accept(*this);
    }
    indent_level--;
}

void PrintVisitor::visit(CrateNode& cr) {
    print_indent();
    out << "CrateNode (name: " << cr.name << ", vis: " << visibility_to_string(cr.vis) << ")\n";
    indent_level++;
    for (auto& var : cr.crate_vars) {
        if (var) var->accept(*this);
    }
    indent_level--;
}

void PrintVisitor::visit(WhileNode& w) {
    print_indent();
    out << "WhileNode\n";
    indent_level++;
    if (w.cond && *w.cond) {
        print_indent();
        out << "Condition:\n";
        indent_level++;
        (*w.cond)->accept(*this);
        indent_level--;
    }
    if (w.body) w.body->accept(*this);
    indent_level--;
}

void PrintVisitor::visit(ForNode& f) {
    print_indent();
    out << "ForNode\n";
    indent_level++;
    if (f.init && *f.init) {
        print_indent();
        out << "Init:\n";
        indent_level++;
        (*f.init)->accept(*this);
        indent_level--;
    }
    if (f.cond && *f.cond) {
        print_indent();
        out << "Cond:\n";
        indent_level++;
        (*f.cond)->accept(*this);
        indent_level--;
    }
    if (f.incr && *f.incr) {
        print_indent();
        out << "Incr:\n";
        indent_level++;
        (*f.incr)->accept(*this);
        indent_level--;
    }
    if (f.for_body) f.for_body->accept(*this);
    indent_level--;
}

void PrintVisitor::visit(IfNode& i) {
    print_indent();
    out << "IfNode\n";
    indent_level++;
    if (i.cond) {
        print_indent();
        out << "Condition:\n";
        indent_level++;
        i.cond->accept(*this);
        indent_level--;
    }
    if (i.body) i.body->accept(*this);
    
    for (auto& ei : i.else_ifs) {
        if (ei) ei->accept(*this);
    }
    if (i.else_node) i.else_node->accept(*this);

    indent_level--;
}

void PrintVisitor::visit(ElseIfNode& i) {
    print_indent();
    out << "ElseIfNode\n";
    indent_level++;
    if (i.cond) i.cond->accept(*this);
    if (i.body) i.body->accept(*this);
    indent_level--;
}

void PrintVisitor::visit(ElseNode& e) {
    print_indent();
    out << "ElseNode\n";
    indent_level++;
    if (e.body) e.body->accept(*this);
    indent_level--;
}

void PrintVisitor::visit(MatchNode& m) {
    print_indent();
    out << "MatchNode\n";
    indent_level++;
    if (m.input) {
        print_indent();
        out << "Input:\n";
        indent_level++;
        m.input->accept(*this);
        indent_level--;
    }
    for (auto& c : m.cases) {
        if (c) c->accept(*this);
    }
    indent_level--;
}

void PrintVisitor::visit(UnaryIncrNode& u) {
    print_indent();
    out << "UnaryIncrNode (op: " << u.op << ")\n";
    indent_level++;
    if (u.name) u.name->accept(*this);
    indent_level--;
}

void PrintVisitor::visit(DotNode& d) {
    print_indent();
    out << "DotNode\n";
    indent_level++;
    if (d.left) d.left->accept(*this);
    if (d.right) d.right->accept(*this);
    indent_level--;
}

void PrintVisitor::visit(CascadeNode& c) {
    print_indent();
    out << "CascadeNode\n";
}

void PrintVisitor::visit(PipelineNode& p) {
    print_indent();
    out << "PipelineNode\n";
    indent_level++;
    if (p.left) p.left->accept(*this);
    if (p.right) p.right->accept(*this);
    indent_level--;
}

void PrintVisitor::visit(ReturnNode& r) {
    print_indent();
    out << "ReturnNode\n";
    indent_level++;
    if (r.ret) r.ret->accept(*this);
    indent_level--;
}

void PrintVisitor::visit(IdentifierCondition& i) {
    print_indent();
    out << "Identifier: " << i.token.token_value << "\n";
}

void PrintVisitor::visit(IntegerCondition& i) {
    print_indent();
    out << "Integer: " << i.token.token_value << "\n";
}

void PrintVisitor::visit(DoubleCondition& d) {
    print_indent();
    out << "Double: " << d.token.token_value << "\n";
}

void PrintVisitor::visit(BoolCondition& b) {
    print_indent();
    out << "Bool: " << b.token.token_value << "\n";
}

void PrintVisitor::visit(StringCondition& s) {
    print_indent();
    out << "String: " << s.token.token_value << "\n";
}

void PrintVisitor::visit(CharCondition& c) {
    print_indent();
    out << "Char: " << c.token.token_value << "\n";
}
