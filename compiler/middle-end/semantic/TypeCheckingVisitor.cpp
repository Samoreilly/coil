
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
bool is_boolean_condition(const Condition* cond, ::Semantic::SymbolTable* table, std::string& resolved_type);

bool is_boolean_condition(const Condition* cond, ::Semantic::SymbolTable* table, std::string& resolved_type) {
    if (!cond) {
        resolved_type.clear();
        return false;
    }

    if (auto* bool_lit = dynamic_cast<const BoolCondition*>(cond)) {
        (void)bool_lit;
        resolved_type = "bool";
        return true;
    }

    auto type = condition_to_type(*cond, table);
    if (type) {
        resolved_type = type->name;
        return resolved_type == "bool";
    }

    resolved_type = resolve_type(cond, table);
    return resolved_type == "bool";
}

std::optional<Type> infer_condition_type(const Condition* cond, ::Semantic::SymbolTable* table) {
    if (!cond) {
        return std::nullopt;
    }

    if (auto direct = condition_to_type(*cond, table)) {
        return direct;
    }

    auto resolved = resolve_type(static_cast<const Node*>(cond), table);
    if (resolved.empty()) {
        return std::nullopt;
    }

    if (auto it = TYPES.find(resolved); it != TYPES.end()) {
        return it->second;
    }

    if (table) {
        if (auto* entry = table->lookup_global(resolved); entry && entry->type) {
            return *entry->type;
        }
    }

    return std::nullopt;
}

std::string describe_condition(const Condition* cond) {
    if (!cond) {
        return "<null>";
    }

    if (auto* ident = dynamic_cast<const IdentifierCondition*>(cond)) {
        return ident->token.token_value;
    }

    if (auto* call = dynamic_cast<const FnCallNode*>(cond)) {
        return call->name + "()";
    }

    if (dynamic_cast<const IntegerCondition*>(cond)) return "int literal";
    if (dynamic_cast<const DoubleCondition*>(cond)) return "double literal";
    if (dynamic_cast<const BoolCondition*>(cond)) return "bool literal";
    if (dynamic_cast<const StringCondition*>(cond)) return "string literal";
    if (dynamic_cast<const CharCondition*>(cond)) return "char literal";

    return "expression";
}

}

bool TypeCheckingVisitor::validate_bool_condition(const Condition* cond, const char* context, ::Semantic::SymbolTable* current_table) {
    std::string resolved_type;

    if (!cond) {
        report_error(std::string(context) + " requires a condition");
        return false;
    }

    if (!is_boolean_condition(cond, current_table, resolved_type)) {
        if (resolved_type.empty()) {
            report_error("Could not resolve " + std::string(context) + " condition type");
        } else {
            report_error(std::string(context) + " condition must be bool, got " + resolved_type);
        }
        return false;
    }

    return true;
}

void TypeCheckingVisitor::visit(GlobalNode& global) {

    for(const auto& node : global.globals) {
        if(node) node->accept(*this);
    }

}

void TypeCheckingVisitor::visit(BodyNode& b, ::Semantic::SymbolTable* current_table) {
    auto* saved_table = table;
    if (current_table) {
        table = current_table;
    }

    for(const auto& node : b.statements) {
        if(node) node->accept(*this);
    }

    table = saved_table;
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

        auto actual_type = condition_to_type(*v.init.value(), table);
        std::string actual_type_name = actual_type ? actual_type->name : resolve_type(*v.init, table);
    
        if (!expected_type.empty() && actual_type && expected_type != actual_type->name) {
            report_error("Type mismatch in variable initialization: expected " + expected_type + ", got " + actual_type_name);
        }

        if (auto* init_match = dynamic_cast<MatchNode*>(v.init.value().get())) {
            const bool saved_expect_expression_yield = expect_expression_yield;
            expect_expression_yield = true;
            init_match->accept(*this);
            expect_expression_yield = saved_expect_expression_yield;
        } else {
            (*v.init)->accept(*this);
        }
    }
}

void TypeCheckingVisitor::visit(FnNode& fn) {
    SymbolTable* fn_scope = table;

    if (table) {
        if (auto* entry = table->lookup_global(fn.name); entry && entry->scope) {
            fn_scope = entry->scope.get();
        }
    }

    if (fn.body) {
        visit(*fn.body, fn_scope);
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
        auto t = infer_condition_type(arg.get(), table); 
        if(!t) {
            report_error("Could not resolve argument '" + describe_condition(arg.get()) + "' in function call: " + b.name);
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
    SymbolEntry* parent = current_table ? current_table->lookup_global(c.name) : nullptr;

    if (!parent || parent->node_kind != NodeKind::CLASS_NODE) {
        report_error("Constructor must belong to a class: " + c.name);
        return;
    }

    if (parent->type && parent->type->name != c.name) {
        report_error("Constructor name: " + c.name + " must match class name: " + parent->type->name);
    }

    auto* ctor_scope = parent && parent->scope ? parent->scope.get() : current_table;
    if (c.body) {
        visit(*c.body, ctor_scope);
    }
}

//overloaded method that defaults to global symboltable
void TypeCheckingVisitor::visit(BodyNode& b) {
   visit(b, table);
}

void TypeCheckingVisitor::visit(ClassNode& c) {
    visit(c, table);
}

void TypeCheckingVisitor::visit(ClassNode& c, ::Semantic::SymbolTable* current_table) {
    SymbolTable* class_scope = current_table;

    if (table) {
        if (auto* entry = table->lookup_global(c.name); entry && entry->scope) {
            class_scope = entry->scope.get();
        }
    }

    if (c.body) {
        visit(*c.body, class_scope);
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

    if (dynamic_cast<const IntegerCondition*>(node)) {
        return "int";
    }

    if (dynamic_cast<const DoubleCondition*>(node)) {
        return "f64";
    }

    if (dynamic_cast<const BoolCondition*>(node)) {
        return "bool";
    }

    if (dynamic_cast<const StringCondition*>(node)) {
        return "string";
    }

    if (dynamic_cast<const CharCondition*>(node)) {
        return "char";
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
        if(!class_entry || !class_entry->scope) {
            report_error("Could not resolve class scope for cascade receiver: " + type);
            continue;
        }

        SymbolEntry* member_entry = class_entry->scope->lookup_local(cas->var_name);
        if(!member_entry || !member_entry->type) {
            report_error("Could not resolve cascade member: " + cas->var_name);
            continue;
        }

        if(member_entry->type->name != t->name) {
           report_error("Types in Cascade do not match: " + t->name);
        }
    }
}


void TypeCheckingVisitor::visit(WhileNode& whl) {
    visit(whl, table);
}

void TypeCheckingVisitor::visit(ForNode& for_node) {
    visit(for_node, table);
}

void TypeCheckingVisitor::visit(IfNode& if_node) {
    visit(if_node, table);
}

void TypeCheckingVisitor::visit(ElseIfNode& else_if_node) {
    visit(else_if_node, table);
}

void TypeCheckingVisitor::visit(ElseNode& else_node) {
    visit(else_node, table);
}

void TypeCheckingVisitor::visit(MatchNode& match_node) {
    visit(match_node, table, expect_expression_yield);
}

void TypeCheckingVisitor::visit(MatchNode& match_node, ::Semantic::SymbolTable* current_table, bool require_yield) {
    auto* scope = current_table ? current_table : table;

    if (!match_node.input) {
        report_error("match requires a selector");
        return;
    }

    auto selector_type = condition_to_type(*match_node.input, scope);
    if (!selector_type) {
        report_error("Could not resolve match selector type");
        return;
    }

    for (const auto& case_item : match_node.cases) {
        if (!case_item.pattern) {
            continue;
        }

        const Token* case_token = nullptr;
        if (auto* ident = dynamic_cast<IdentifierCondition*>(case_item.pattern.get())) {
            case_token = &ident->token;
        } else if (auto* integer = dynamic_cast<IntegerCondition*>(case_item.pattern.get())) {
            case_token = &integer->token;
        } else if (auto* dbl = dynamic_cast<DoubleCondition*>(case_item.pattern.get())) {
            case_token = &dbl->token;
        } else if (auto* boolean = dynamic_cast<BoolCondition*>(case_item.pattern.get())) {
            case_token = &boolean->token;
        } else if (auto* string = dynamic_cast<StringCondition*>(case_item.pattern.get())) {
            case_token = &string->token;
        } else if (auto* chr = dynamic_cast<CharCondition*>(case_item.pattern.get())) {
            case_token = &chr->token;
        }

        if (auto* ident = dynamic_cast<IdentifierCondition*>(case_item.pattern.get()); ident && ident->token.token_value == "_") {
            // wildcard arm is always allowed
        } else {
            auto pattern_type = condition_to_type(*case_item.pattern, scope);
            if (!pattern_type) {
                report_error("Could not resolve match case type", case_token ? *case_token : Token{});
                continue;
            }

            if (!same_type(*selector_type, *pattern_type) && !common_numeric_type(*selector_type, *pattern_type)) {
                report_error("Match case type does not match selector type: expected " + selector_type->name + ", got " + pattern_type->name, case_token ? *case_token : Token{});
            }
        }

        if (case_item.body) {
            auto case_scope = std::make_shared<SymbolTable>("match_case");
            case_scope->parent = scope;
            auto* saved_table = table;
            table = case_scope.get();

            for (const auto& stmt : case_item.body->statements) {
                if (stmt) {
                    if (auto* var = dynamic_cast<VariableNode*>(stmt.get())) {
                        auto* ident = dynamic_cast<IdentifierCondition*>(var->name.get());
                        if (!ident) {
                            stmt->accept(*this);
                        } else {
                            std::optional<Type> local_type = var->type;
                            if (!local_type && var->init && *var->init) {
                                local_type = condition_to_type(*var->init.value(), table);
                            }

                            if (local_type) {
                                SymbolEntry entry(
                                    NodeKind::VARIABLE_NODE,
                                    var->vis,
                                    var->access,
                                    *local_type,
                                    {},
                                    {},
                                    0,
                                    ident->token
                                );

                                case_scope->insert(ident->token.token_value, entry);
                            }

                            stmt->accept(*this);
                        }
                    } else {
                        stmt->accept(*this);
                    }
                }
            }

            if (require_yield && !body_contains_yield(*case_item.body)) {
                report_error("All match arms must yield when match is used as an expression", case_token ? *case_token : Token{});
            }

            table = saved_table;
        }
    }
}

void TypeCheckingVisitor::visit(YieldNode& yield_node) {
    if (yield_node.value) {
        auto yielded_type = condition_to_type(*yield_node.value, table);
        if (!yielded_type) {
            report_error("Could not resolve yield value type");
            return;
        }

        yield_node.value->accept(*this);
    }
}

void TypeCheckingVisitor::visit(WhileNode& whl, ::Semantic::SymbolTable* current_table) {
    auto* scope = current_table ? current_table : table;

    validate_bool_condition(whl.cond ? whl.cond->get() : nullptr, "while", scope);

    if (whl.body) {
        visit(*whl.body, scope);
    }
}

void TypeCheckingVisitor::visit(ForNode& for_node, ::Semantic::SymbolTable* current_table) {
    auto* scope = current_table ? current_table : table;

    if (for_node.init && *for_node.init) {
        (*for_node.init)->accept(*this);
    }

    if (for_node.cond && *for_node.cond) {
        validate_bool_condition(for_node.cond->get(), "for", scope);
    }

    if (for_node.incr && *for_node.incr) {
        (*for_node.incr)->accept(*this);
    }

    if (for_node.for_body) {
        visit(*for_node.for_body, scope);
    }
}

void TypeCheckingVisitor::visit(IfNode& if_node, ::Semantic::SymbolTable* current_table) {
    auto* scope = current_table ? current_table : table;

    validate_bool_condition(if_node.cond.get(), "if", scope);

    if (if_node.body) {
        visit(*if_node.body, scope);
    }

    for (const auto& else_if : if_node.else_ifs) {
        if (else_if) {
            visit(*else_if, scope);
        }
    }

    if (if_node.else_node) {
        visit(*if_node.else_node, scope);
    }
}

void TypeCheckingVisitor::visit(ElseIfNode& else_if_node, ::Semantic::SymbolTable* current_table) {
    auto* scope = current_table ? current_table : table;

    validate_bool_condition(else_if_node.cond.get(), "elseif", scope);

    if (else_if_node.body) {
        visit(*else_if_node.body, scope);
    }
}

void TypeCheckingVisitor::visit(ElseNode& else_node, ::Semantic::SymbolTable* current_table) {
    auto* scope = current_table ? current_table : table;

    if (else_node.body) {
        visit(*else_node.body, scope);
    }
}
