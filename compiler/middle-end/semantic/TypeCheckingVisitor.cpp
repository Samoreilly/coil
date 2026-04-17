
#include <fmt/core.h>
#include "TypeCheckingVisitor.h"
#include "../../frontend/lexer/Token.h"
#include <algorithm>

using SymbolTable = ::Semantic::SymbolTable;
using SymbolEntry = ::Semantic::SymbolEntry;

void TypeCheckingVisitor::report_error(std::string message) {
    diagnostics.send(DiagPhase::SEMANTIC, std::move(message));
}

void TypeCheckingVisitor::report_error(std::string message, const Token& token) {
    diagnostics.send(
        DiagPhase::SEMANTIC,
        std::move(message),
        token.line,
        token.col,
        token.token_value,
        token.file_name
    );
}

namespace {

std::string resolve_type(const Node* node, ::Semantic::SymbolTable* current_table);
std::string resolve_type(const std::unique_ptr<Condition>& con, ::Semantic::SymbolTable* current_table);

std::string infer_type_name(const Condition& c, ::Semantic::SymbolTable* table) {
    if (auto t = condition_to_type(c, table)) {
        return t->name;
    }

    return resolve_type(&c, table);
}

}
void TypeCheckingVisitor::visit(GlobalNode& global) {

    for(const auto& node : global.globals) {
        if(node) node->accept(*this);
    }

}

void TypeCheckingVisitor::visit(BodyNode& b, ::Semantic::SymbolTable* current_table) {
    (void)current_table;
    for(const auto& node : b.statements) {
        if(node) node->accept(*this);
    }
}

void TypeCheckingVisitor::visit(VariableNode& v) {
    
    auto is_known_type = [&](const std::string& type_name) {
        if (TYPES.find(type_name) != TYPES.end()) {
            return true;
        }

        if (table) {
            if (auto* entry = table->lookup_global(type_name)) {
                return entry->node_kind == NodeKind::CLASS_NODE;
            }
        }

        return false;
    };

    std::string declared_name;
    if (auto* ident = dynamic_cast<IdentifierCondition*>(v.name.get())) {
        declared_name = ident->token.token_value;
    }

    if (!declared_name.empty() && is_known_type(declared_name)) {
        report_error("Redefinition of type name: " + declared_name);
    }

    if (v.init && *v.init) {
        std::string expected_type;
   
        if (v.type) {
            expected_type = v.type->name;
        } else if (v.op) {
            expected_type = resolve_type(v.name, table);
        }

        std::string init_type = resolve_type(*v.init, table);
        auto actual_type = condition_to_type(*v.init.value(), table);
   
        if (!expected_type.empty() && actual_type && expected_type != actual_type->name) {
            fmt::println(stderr, "{} : {}", init_type, expected_type);
            report_error("Type mismatch in variable initialization: expected " + expected_type + ", got " + init_type);
        }
        
        (*v.init)->accept(*this);
    }
}

void TypeCheckingVisitor::visit(FnCallNode& b) {
    
    SymbolEntry* parent = table->lookup_local(b.name);
    
    //constructor node indistinguishable from function call in parsing
    if(parent && parent->node_kind == NodeKind::CLASS_NODE) {
        report_error("Function name conflicts with class name: " + b.name);    
    }

    std::vector<Type> true_arg_types;
    for(const auto& arg: b.arguments) {
        auto t = condition_to_type(*arg, table); 
        if(!t) {
            report_error("Could not resolve argument types from function call: " + b.name);
            return;
        }

        true_arg_types.push_back(*t);
    }


    SymbolEntry* fn_signature = table->lookup_global(b.name);
    if(!fn_signature) return;


    //compare argument types to parameter types
    auto same_type = [](const Type& a, const Type& b) {
        return a.type == b.type &&
            a.bit_width == b.bit_width &&
            a.is_signed == b.is_signed &&
            a.name == b.name;
    };
    
    if(!std::equal(
        fn_signature->param_types.begin(),
        fn_signature->param_types.end(),
        true_arg_types.begin(),
        true_arg_types.end(),
        same_type)) {
        report_error("Could not resolve argument types in function call: " + b.name);
        return;
    }

    
}

//overloaded method that defaults to global symboltable
void TypeCheckingVisitor::visit(ConstructorNode& c) {
    visit(c, table);
}

void TypeCheckingVisitor::visit(ConstructorNode& c, ::Semantic::SymbolTable* current_table) {
    SymbolEntry* parent = current_table->lookup_global(c.name);

    if (!parent || parent->node_kind != NodeKind::CLASS_NODE) {
        report_error("Constructor must belong to a class: " + c.name);
        return;
    }

    if (parent->type && parent->type->name != c.name) {
        report_error("Constructor name: " + c.name + " must match class name: " + parent->type->name);
    }
}

//overloaded method that defaults to global symboltable
void TypeCheckingVisitor::visit(BodyNode& b) {
   visit(b, table);
}

void TypeCheckingVisitor::visit(ClassNode& c) {
    SymbolTable class_scope(c.name);
    visit(c, &class_scope);
}

void TypeCheckingVisitor::visit(ClassNode& c, ::Semantic::SymbolTable* current_table) {
    if (c.body) {
        visit(*c.body, current_table);
    }
}

void TypeCheckingVisitor::visit(CascadeNode& id) {
    visit(id, table);
}

namespace {

std::string resolve_type(const Node* node, ::Semantic::SymbolTable* current_table);

std::string resolve_type(const std::unique_ptr<Condition>& con, ::Semantic::SymbolTable* current_table) {
    return resolve_type(con.get(), current_table);
}

std::string resolve_type(const Node* node, ::Semantic::SymbolTable* current_table) {
    if (!node) {
        return {};
    }

    if (!current_table) {
        return {};
    }

    if (auto* ident = dynamic_cast<const IdentifierCondition*>(node)) {
        if (auto* entry = current_table->lookup_local(ident->token.token_value)) {
            if (entry->type) {
                return entry->type->name;
            }
        }

        if (auto* entry = current_table->lookup_global(ident->token.token_value)) {
            if (entry->type) {
                return entry->type->name;
            }
        }

        return ident->token.token_value;
    }

    if (auto* call = dynamic_cast<const FnCallNode*>(node)) {
        if (auto* entry = current_table->lookup_global(call->name)) {
            if (entry->type) {
                return entry->type->name;
            }
        }

        return call->name;
    }

    if (auto* ctor = dynamic_cast<const ConstructorNode*>(node)) {
        return ctor->name;
    }

    if (auto* var = dynamic_cast<const VariableNode*>(node)) {
        return resolve_type(var->name, current_table);
    }

    if (auto* ret = dynamic_cast<const ReturnNode*>(node)) {
        return resolve_type(ret->ret, current_table);
    }

    if (auto* bin = dynamic_cast<const BinaryExpression*>(node)) {
        auto lhs = resolve_type(bin->left, current_table);
        if (!lhs.empty()) {
            return lhs;
        }

        return resolve_type(bin->right, current_table);
    }

    if (auto* dot = dynamic_cast<const DotNode*>(node)) {
        auto lhs = resolve_type(dot->left.get(), current_table);
        if (lhs.empty()) {
            return {};
        }

        auto* lhs_entry = current_table->lookup_global(lhs);
        if (!lhs_entry || !lhs_entry->scope) {
            return {};
        }

        return resolve_type(dot->right.get(), lhs_entry->scope.get());
    }

    if (auto* cascade = dynamic_cast<const CascadeNode*>(node)) {
        return resolve_type(cascade->name, current_table);
    }

    if (auto* unary = dynamic_cast<const UnaryIncrNode*>(node)) {
        return resolve_type(unary->name, current_table);
    }

    return {};
}

} // namespace

void TypeCheckingVisitor::visit(CascadeNode& id, ::Semantic::SymbolTable* current_table) {
    for(const auto& cas : id.cascades) {
        if (!cas->condition) {
            continue;
        }

        std::optional<Type> t = condition_to_type(*cas->condition, current_table);
        if(!t.has_value()) {
            report_error("Could not resolve type for cascade condition");
            continue;
        }

        auto type = resolve_type(id.name, current_table);
        if (type.empty()) {
            report_error("Could not resolve cascade receiver type");
            continue;
        }

        SymbolEntry* class_entry = current_table ? current_table->lookup_global(type) : nullptr;
        if (!class_entry || !class_entry->scope) {
            report_error("Could not resolve class scope for cascade receiver: " + type);
            continue;
        }

        SymbolEntry* member_entry = class_entry->scope->lookup_local(cas->var_name);
        if (!member_entry || !member_entry->type) {
            report_error("Could not resolve cascade member: " + cas->var_name);
            continue;
        }

        if (member_entry->type->name != t->name) {
           report_error("Types in Cascade do not match: " + t->name);
        }
    }
}
