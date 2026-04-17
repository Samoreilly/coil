
#include <fmt/core.h>
#include "../../ast/Node.h"
#include "RegisterVisitor.h"
#include "TypeAnalysis.h"
#include "../../Support/ErrorHandling/Error.h"

namespace {

using SymbolTable = ::Semantic::SymbolTable;
using SymbolEntry = ::Semantic::SymbolEntry;

std::string declared_name(const std::unique_ptr<Condition>& condition) {
    if (!condition) {
        return {};
    }

    if (auto* ident = dynamic_cast<IdentifierCondition*>(condition.get())) {
        return ident->token.token_value;
    }

    return {};
}

Token declared_token(const std::unique_ptr<Condition>& condition) {
    if (!condition) {
        return {};
    }

    if (auto* ident = dynamic_cast<IdentifierCondition*>(condition.get())) {
        return ident->token;
    }

    return {};
}

bool same_signature(const std::vector<Type>& lhs, const std::vector<Type>& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t i = 0; i < lhs.size(); ++i) {
        if (!same_type(lhs[i], rhs[i])) {
            return false;
        }
    }

    return true;
}

std::string constructor_key(const std::string& name, const std::vector<Type>& params) {
    std::string key = "ctor:" + name + "#";

    for (const auto& type : params) {
        key += std::to_string(static_cast<int>(type.type));
        key += ':';
        key += std::to_string(type.bit_width);
        key += ':';
        key += type.is_signed ? "1" : "0";
        key += ':';
        key += type.name;
        key += '|';
    }

    return key;
}

void duplicate_error(Diagnostics& diagnostics, const std::string& kind, const std::string& name, const Token* token = nullptr) {
    const int line = token ? token->line : -1;
    const int col = token ? token->col : -1;
    const std::string value = token ? token->token_value : "";
    const std::string file = token ? token->file_name : "";

    DiagInfo diag(
        DiagPhase::SEMANTIC,
        "Redefinition of " + kind + " '" + name + "'",
        line,
        col,
        value,
        file
    );

    diagnostics.send(diag);
}

void semantic_error(Diagnostics& diagnostics, const std::string& message, const Token* token = nullptr) {
    const int line = token ? token->line : -1;
    const int col = token ? token->col : -1;
    const std::string value = token ? token->token_value : "";
    const std::string file = token ? token->file_name : "";

    diagnostics.send(
        DiagPhase::SEMANTIC,
        message,
        line,
        col,
        value,
        file
    );
}

} // namespace

void RegisterVisitor::visit(GlobalNode& global) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(global, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(GlobalNode& global, ::Semantic::SymbolTable* current_table, int& current_offset) {
    auto* saved_table = active_table;
    int saved_offset = active_offset;

    active_table = current_table;
    active_offset = current_offset;

    for (const auto& node : global.globals) {
        if (node) {
            node->accept(*this);
        }
    }

    current_offset = active_offset;
    active_table = saved_table;
    active_offset = saved_offset;
}

void RegisterVisitor::visit(VariableNode& v) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(v, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(VariableNode& v, ::Semantic::SymbolTable* current_table, int& current_offset) {
    auto* saved_table = active_table;
    int saved_offset = active_offset;

    active_table = current_table;
    active_offset = current_offset;

    auto restore = [&]() {
        active_table = saved_table;
        active_offset = saved_offset;
    };

    if (!active_table) {
        restore();
        return;
    }

    if (v.inferred_type && !v.type) {
        if (!v.init || !*v.init) {
            Token name_token = declared_token(v.name);
            semantic_error(diagnostics, "auto variable requires an initializer", &name_token);
            restore();
            return;
        }

        auto inferred = condition_to_type(*v.init.value(), active_table);
        if (!inferred) {
            Token name_token = declared_token(v.name);
            semantic_error(diagnostics, "Could not infer type for auto variable", &name_token);
            restore();
            return;
        }

        v.type = *inferred;
    }

    if (!v.type) {
        restore();
        return;
    }

    std::string name = declared_name(v.name);
    if (name.empty()) {
        restore();
        return;
    }

    if (active_table->lookup_local(name) != nullptr) {
        duplicate_error(diagnostics, "variable", name);
    } else {
        SymbolEntry entry(
            NodeKind::VARIABLE_NODE,
            v.vis,
            v.access,
            v.type,
            {},
            {},
            active_offset,
            declared_token(v.name)
        );

        active_table->insert(name, entry);
        active_offset += 1;
    }

    current_offset = active_offset;
    restore();
}

void RegisterVisitor::visit(BinaryExpression& b) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(b, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(BinaryExpression& b, ::Semantic::SymbolTable* current_table, int& current_offset) {
    (void)b;
    (void)current_table;
    (void)current_offset;
}

void RegisterVisitor::visit(FnNode& f) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(f, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(FnNode& f, ::Semantic::SymbolTable* current_table, int& current_offset) {
    auto* saved_table = active_table;
    int saved_offset = active_offset;

    active_table = current_table;
    active_offset = current_offset;

    auto restore = [&]() {
        active_table = saved_table;
        active_offset = saved_offset;
    };

    if (!active_table) {
        restore();
        return;
    }

    if (active_table->lookup_local(f.name) != nullptr) {
        duplicate_error(diagnostics, "function", f.name);
    } else {
        SymbolEntry entry(
            NodeKind::FN_NODE,
            f.vis,
            ACCESS::MUTABLE,
            f.return_type,
            parameter_types(f.parameters),
            {},
            active_offset
        );

        entry.token = Token{TokenType::FN, f.name, "", -1, -1};

        active_table->insert(f.name, entry);
    }

    auto fn_scope = std::make_shared<SymbolTable>(f.name);
    fn_scope->parent = active_table;

    int fn_offset = 0;
    for (const auto& param : f.parameters) {
        if (!param) {
            continue;
        }

        if (fn_scope->lookup_local(param->name) != nullptr) {
            duplicate_error(diagnostics, "parameter", param->name);
            continue;
        }

        SymbolEntry param_entry(
            NodeKind::VARIABLE_NODE,
            Visibility::PUBLIC,
            ACCESS::MUTABLE,
            parameter_type(param),
            {},
            {},
            fn_offset
        );

        fn_scope->insert(param->name, param_entry);
        fn_offset += 1;
    }

    if (active_table->lookup_local(f.name) != nullptr) {
        active_table->entries[f.name].scope = fn_scope;
    }

    if (f.body) {
        visit(*f.body, fn_scope.get(), fn_offset);
    }

    if (active_table->lookup_local(f.name) != nullptr) {
        active_table->entries[f.name].offset = fn_offset;
    }

    current_offset = active_offset;
    restore();
}

void RegisterVisitor::visit(FnCallNode& f) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(f, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(FnCallNode& f, ::Semantic::SymbolTable* current_table, int& current_offset) {
    (void)f;
    (void)current_table;
    (void)current_offset;
}

void RegisterVisitor::visit(ConstructorNode& c) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(c, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(ConstructorNode& c, ::Semantic::SymbolTable* current_table, int& current_offset) {
    auto* saved_table = active_table;
    int saved_offset = active_offset;

    active_table = current_table;
    active_offset = current_offset;

    auto restore = [&]() {
        active_table = saved_table;
        active_offset = saved_offset;
    };

    if (!active_table) {
        restore();
        return;
    }

    const auto param_types = parameter_types(c.params);
    bool duplicate_constructor = false;

    //checks for constructor with same parameters
    for (const auto& [name, entry] : active_table->entries) {
        (void)name;
        if (entry.node_kind == NodeKind::CONSTRUCTOR_NODE && entry.type
            && entry.type->name == c.name && same_signature(entry.param_types, param_types)) {
            
            duplicate_error(diagnostics, "constructor", c.name, &entry.token);
            duplicate_constructor = true;
            break;
        }
    }

    SymbolEntry* constructor_entry = nullptr;
    if (!duplicate_constructor) {
        SymbolEntry entry(
            NodeKind::CONSTRUCTOR_NODE,
            c.vis,
            ACCESS::MUTABLE,
            Type{TypeCategory::CLASS, 0, false, c.name},
            param_types,
            {},
            active_offset
        );

        entry.token = Token{TokenType::CONSTRUCTOR, c.name, "", -1, -1};

        constructor_entry = &active_table->entries.emplace(
            constructor_key(c.name, param_types),
            std::move(entry)
        ).first->second;
    }

    auto constructor_scope = std::make_shared<SymbolTable>(c.name);
    constructor_scope->parent = active_table;

    int constructor_offset = 0;
    for (const auto& param : c.params) {
        if (!param) {
            continue;
        }

        if (constructor_scope->lookup_local(param->name) != nullptr) {
            duplicate_error(diagnostics, "parameter", param->name);
            continue;
        }

        SymbolEntry param_entry(
            NodeKind::VARIABLE_NODE,
            Visibility::PUBLIC,
            ACCESS::MUTABLE,
            parameter_type(param),
            {},
            {},
            constructor_offset
        );

        constructor_scope->insert(param->name, param_entry);
        constructor_offset += 1;
    }

    if (constructor_entry != nullptr) {
        constructor_entry->scope = constructor_scope;
    }

    if (c.body) {
        visit(*c.body, constructor_scope.get(), constructor_offset);
    }

    if (constructor_entry != nullptr) {
        constructor_entry->offset = constructor_offset;
    }

    current_offset = active_offset;
    restore();
}

void RegisterVisitor::visit(BodyNode& b) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(b, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(BodyNode& b, ::Semantic::SymbolTable* current_table, int& current_offset) {
    auto* saved_table = active_table;
    int saved_offset = active_offset;

    active_table = current_table;
    active_offset = current_offset;

    for (const auto& stmt : b.statements) {
        if (stmt) {
            stmt->accept(*this);
        }
    }

    current_offset = active_offset;
    active_table = saved_table;
    active_offset = saved_offset;
}

void RegisterVisitor::visit(ClassNode& c) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(c, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(ClassNode& c, ::Semantic::SymbolTable* current_table, int& current_offset) {
    auto* saved_table = active_table;
    int saved_offset = active_offset;

    active_table = current_table;
    active_offset = current_offset;

    auto restore = [&]() {
        active_table = saved_table;
        active_offset = saved_offset;
    };

    if (!active_table) {
        restore();
        return;
    }

    if (active_table->lookup_local(c.name) != nullptr) {
        duplicate_error(diagnostics, "class", c.name);
    } else {
        SymbolEntry entry(
            NodeKind::CLASS_NODE,
            c.vis,
            ACCESS::MUTABLE,
            Type{TypeCategory::CLASS, 0, false, c.name},
            {},
            {},
            active_offset
        );

        entry.token = Token{TokenType::CLASS, c.name, "", -1, -1};

        active_table->insert(c.name, entry);
    }

    auto class_scope = std::make_shared<SymbolTable>(c.name);
    class_scope->parent = active_table;
    if (active_table->lookup_local(c.name) != nullptr) {
        active_table->entries[c.name].scope = class_scope;
    }

    int class_offset = 0;
    if (c.body) {
        visit(*c.body, class_scope.get(), class_offset);
    }

    if (active_table->lookup_local(c.name) != nullptr) {
        active_table->entries[c.name].offset = class_offset;
    }

    current_offset = active_offset;
    restore();
}

void RegisterVisitor::visit(CrateNode& cr) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(cr, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(CrateNode& cr, ::Semantic::SymbolTable* current_table, int& current_offset) {
    auto* saved_table = active_table;
    int saved_offset = active_offset;

    active_table = current_table;
    active_offset = current_offset;

    auto restore = [&]() {
        active_table = saved_table;
        active_offset = saved_offset;
    };

    if (!active_table) {
        restore();
        return;
    }

    if (active_table->lookup_local(cr.name) != nullptr) {
        duplicate_error(diagnostics, "crate", cr.name);
    } else {
        SymbolEntry entry(
            NodeKind::CRATE_NODE,
            cr.vis,
            ACCESS::MUTABLE,
            Type{TypeCategory::CRATE, 0, false, cr.name},
            {},
            {},
            active_offset
        );

        entry.token = Token{TokenType::CRATE, cr.name, "", -1, -1};

        active_table->insert(cr.name, entry);
    }

    auto crate_scope = std::make_shared<SymbolTable>(cr.name);
    crate_scope->parent = active_table;
    if (active_table->lookup_local(cr.name) != nullptr) {
        active_table->entries[cr.name].scope = crate_scope;
    }

    int crate_offset = 0;
    for (const auto& var : cr.crate_vars) {
        if (var) {
            visit(*var, crate_scope.get(), crate_offset);
        }
    }

    if (active_table->lookup_local(cr.name) != nullptr) {
        active_table->entries[cr.name].offset = crate_offset;
    }

    current_offset = active_offset;
    restore();
}

void RegisterVisitor::visit(WhileNode& w) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(w, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(WhileNode& w, ::Semantic::SymbolTable* current_table, int& current_offset) {
    auto* saved_table = active_table;
    int saved_offset = active_offset;

    active_table = current_table;
    active_offset = current_offset;

    if (w.body) {
        w.body->accept(*this);
    }

    current_offset = active_offset;
    active_table = saved_table;
    active_offset = saved_offset;
}

void RegisterVisitor::visit(ForNode& f) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(f, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(ForNode& f, ::Semantic::SymbolTable* current_table, int& current_offset) {
    auto* saved_table = active_table;
    int saved_offset = active_offset;

    active_table = current_table;
    active_offset = current_offset;

    if (f.init && *f.init) {
        (*f.init)->accept(*this);
    }

    if (f.for_body) {
        f.for_body->accept(*this);
    }

    current_offset = active_offset;
    active_table = saved_table;
    active_offset = saved_offset;
}

void RegisterVisitor::visit(IfNode& i) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(i, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(IfNode& i, ::Semantic::SymbolTable* current_table, int& current_offset) {
    auto* saved_table = active_table;
    int saved_offset = active_offset;

    active_table = current_table;
    active_offset = current_offset;

    if (i.body) {
        i.body->accept(*this);
    }

    for (const auto& else_if : i.else_ifs) {
        if (else_if) {
            else_if->accept(*this);
        }
    }

    if (i.else_node) {
        i.else_node->accept(*this);
    }

    current_offset = active_offset;
    active_table = saved_table;
    active_offset = saved_offset;
}

void RegisterVisitor::visit(ElseIfNode& i) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(i, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(ElseIfNode& i, ::Semantic::SymbolTable* current_table, int& current_offset) {
    auto* saved_table = active_table;
    int saved_offset = active_offset;

    active_table = current_table;
    active_offset = current_offset;

    if (i.body) {
        i.body->accept(*this);
    }

    current_offset = active_offset;
    active_table = saved_table;
    active_offset = saved_offset;
}

void RegisterVisitor::visit(ElseNode& e) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(e, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(ElseNode& e, ::Semantic::SymbolTable* current_table, int& current_offset) {
    auto* saved_table = active_table;
    int saved_offset = active_offset;

    active_table = current_table;
    active_offset = current_offset;

    if (e.body) {
        e.body->accept(*this);
    }

    current_offset = active_offset;
    active_table = saved_table;
    active_offset = saved_offset;
}

void RegisterVisitor::visit(MatchNode& m) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(m, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(MatchNode& m, ::Semantic::SymbolTable* current_table, int& current_offset) {
    (void)m;
    (void)current_table;
    (void)current_offset;
}

void RegisterVisitor::visit(UnaryIncrNode& u) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(u, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(UnaryIncrNode& u, ::Semantic::SymbolTable* current_table, int& current_offset) {
    (void)u;
    (void)current_table;
    (void)current_offset;
}

void RegisterVisitor::visit(DotNode& d) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(d, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(DotNode& d, ::Semantic::SymbolTable* current_table, int& current_offset) {
    (void)d;
    (void)current_table;
    (void)current_offset;
}

void RegisterVisitor::visit(CascadeNode& c) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(c, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(CascadeNode& c, ::Semantic::SymbolTable* current_table, int& current_offset) {
    (void)c;
    (void)current_table;
    (void)current_offset;
}

void RegisterVisitor::visit(PipelineNode& p) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(p, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(PipelineNode& p, ::Semantic::SymbolTable* current_table, int& current_offset) {
    (void)p;
    (void)current_table;
    (void)current_offset;
}

void RegisterVisitor::visit(ReturnNode& r) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(r, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(ReturnNode& r, ::Semantic::SymbolTable* current_table, int& current_offset) {
    (void)r;
    (void)current_table;
    (void)current_offset;
}

void RegisterVisitor::visit(ConversionNode& c) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(c, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(ConversionNode& c, ::Semantic::SymbolTable* current_table, int& current_offset) {
    (void)c;
    (void)current_table;
    (void)current_offset;
}

void RegisterVisitor::visit(IdentifierCondition& i) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(i, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(IdentifierCondition& i, ::Semantic::SymbolTable* current_table, int& current_offset) {
    (void)i;
    (void)current_table;
    (void)current_offset;
}

void RegisterVisitor::visit(IntegerCondition& i) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(i, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(IntegerCondition& i, ::Semantic::SymbolTable* current_table, int& current_offset) {
    (void)i;
    (void)current_table;
    (void)current_offset;
}

void RegisterVisitor::visit(DoubleCondition& d) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(d, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(DoubleCondition& d, ::Semantic::SymbolTable* current_table, int& current_offset) {
    (void)d;
    (void)current_table;
    (void)current_offset;
}

void RegisterVisitor::visit(BoolCondition& b) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(b, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(BoolCondition& b, ::Semantic::SymbolTable* current_table, int& current_offset) {
    (void)b;
    (void)current_table;
    (void)current_offset;
}

void RegisterVisitor::visit(StringCondition& s) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(s, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(StringCondition& s, ::Semantic::SymbolTable* current_table, int& current_offset) {
    (void)s;
    (void)current_table;
    (void)current_offset;
}

void RegisterVisitor::visit(CharCondition& c) {
    auto* scope = active_table ? active_table : table;
    int offset = active_offset;
    visit(c, scope, offset);
    active_offset = offset;
}

void RegisterVisitor::visit(CharCondition& c, ::Semantic::SymbolTable* current_table, int& current_offset) {
    (void)c;
    (void)current_table;
    (void)current_offset;
}
