
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

std::optional<Type> infer_type(const Condition* cond, ::Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input = std::nullopt);
bool is_boolean_condition(const Condition* cond, ::Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input, std::string& resolved_type);
const SymbolEntry* resolve_constructor_entry(::Semantic::SymbolTable* table, const std::string& class_name, const std::vector<Type>& arg_types);

bool same_signature(const std::vector<Type>& lhs, const std::vector<Type>& rhs) {
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), same_type);
}

const SymbolEntry* resolve_constructor_entry(::Semantic::SymbolTable* table, const std::string& class_name, const std::vector<Type>& arg_types) {
    if (!table) {
        return nullptr;
    }

    auto* class_entry = table->lookup_global(class_name);
    if (!class_entry || !class_entry->scope) {
        return nullptr;
    }

    for (const auto& [key, entry] : class_entry->scope->entries) {
        (void)key;
        if (entry.node_kind != NodeKind::CONSTRUCTOR_NODE || !entry.type || entry.type->name != class_name) {
            continue;
        }

        if (same_signature(entry.param_types, arg_types)) {
            return &entry;
        }
    }

    return nullptr;
}

bool is_boolean_condition(const Condition* cond, ::Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input, std::string& resolved_type) {
    if (!cond) {
        resolved_type.clear();
        return false;
    }

    if (auto* bool_lit = dynamic_cast<const BoolCondition*>(cond)) {
        (void)bool_lit;
        resolved_type = "bool";
        return true;
    }

    auto type = condition_to_type(*cond, table, pipeline_input);
    if (type) {
        resolved_type = type->name;
        return resolved_type == "bool";
    }

    auto inferred = infer_type(cond, table, pipeline_input);
    resolved_type = inferred ? inferred->name : std::string{};
    return resolved_type == "bool";
}

std::optional<Type> infer_type(const Condition* cond, ::Semantic::SymbolTable* table, const std::optional<Type>& pipeline_input) {
    if (!cond) {
        return std::nullopt;
    }

    if (auto direct = condition_to_type(*cond, table, pipeline_input)) {
        return direct;
    }

    if (auto* bin = dynamic_cast<const BinaryExpression*>(cond)) {
        if (bin->left) {
            if (auto left = infer_type(bin->left.get(), table, pipeline_input)) {
                return left;
            }
        }

        if (bin->right) {
            return infer_type(bin->right.get(), table, pipeline_input);
        }
    }

    if (auto* pipe = dynamic_cast<const PipelineNode*>(cond)) {
        if (pipe->left) {
            return infer_type(pipe->left.get(), table, pipeline_input);
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
    bool ok = true;

    if (!cond) {
        report_error(std::string(context) + " requires a condition");
        return false;
    }

    if (!is_boolean_condition(cond, current_table, current_pipeline_input, resolved_type)) {
        if (resolved_type.empty()) {
            report_error("Could not resolve " + std::string(context) + " condition type");
        } else {
            report_error(std::string(context) + " condition must be bool, got " + resolved_type);
        }
        ok = false;
    }

    auto* saved_table = table;
    if (current_table) {
        table = current_table;
    }

    if (cond) {
        const_cast<Condition*>(cond)->accept(*this);
    }

    table = saved_table;

    return ok;
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
        auto expected_type = v.type;
        if (!expected_type && v.op) {
            expected_type = condition_to_type(*v.name, table, current_pipeline_input);
        }

        auto actual_type = infer_type(v.init.value().get(), table, current_pipeline_input);
        if (!expected_type && actual_type && v.op) {
            expected_type = actual_type;
        }

        if (expected_type && actual_type && !same_type(*expected_type, *actual_type) && !can_implicitly_convert(*actual_type, *expected_type)) {
            report_error("Type mismatch in variable initialization: expected " + expected_type->name + ", got " + actual_type->name);
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
    const auto saved_return_type = current_return_type;

    if (table) {
        if (auto* entry = table->lookup_global(fn.name); entry && entry->scope) {
            fn_scope = entry->scope.get();
        }
    }

    current_return_type = fn.return_type;

    if (fn.body) {
        visit(*fn.body, fn_scope);
    }

    current_return_type = saved_return_type;
}

void TypeCheckingVisitor::visit(FnCallNode& b) {
    std::vector<Type> true_arg_types;
    for(const auto& arg: b.arguments) {
        auto t = infer_type(arg.get(), table, current_pipeline_input); 
        if(!t) {
            report_error("Could not resolve argument '" + describe_condition(arg.get()) + "' in function call: " + b.name);
            if (arg) {
                const_cast<Condition*>(arg.get())->accept(*this);
            }
            return;
        }

        true_arg_types.push_back(*t);
    }

    SymbolEntry* fn_signature = table ? table->lookup_global(b.name) : nullptr;
    if (fn_signature && fn_signature->node_kind == NodeKind::CLASS_NODE) {
        const auto* constructor_entry = resolve_constructor_entry(table, b.name, true_arg_types);
        if (!constructor_entry) {
            report_error("Could not resolve constructor types in function call: " + b.name);
            for (const auto& arg : b.arguments) {
                if (arg) {
                    const_cast<Condition*>(arg.get())->accept(*this);
                }
            }
            return;
        }

        fn_signature = const_cast<SymbolEntry*>(constructor_entry);
    }

    if (!fn_signature) {
        for (const auto& arg : b.arguments) {
            if (arg) {
                const_cast<Condition*>(arg.get())->accept(*this);
            }
        }
        return;
    }

    if (!same_signature(fn_signature->param_types, true_arg_types)) {
        report_error("Could not resolve argument types in function call: " + b.name);
        for (const auto& arg : b.arguments) {
            if (arg) {
                const_cast<Condition*>(arg.get())->accept(*this);
            }
        }
        return;
    }

    for (const auto& arg : b.arguments) {
        if (arg) {
            const_cast<Condition*>(arg.get())->accept(*this);
        }
    }

    
}

void TypeCheckingVisitor::visit(BinaryExpression& b) {
    if (b.left) {
        b.left->accept(*this);
    }

    if (b.right) {
        b.right->accept(*this);
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

void TypeCheckingVisitor::visit(CascadeNode& id, ::Semantic::SymbolTable* current_table) {
    auto* saved_table = table;
    if (current_table) {
        table = current_table;
    }

    if (id.name) {
        const_cast<Condition*>(id.name.get())->accept(*this);
    }

    for(const auto& cas : id.cascades) {
        if (!cas->condition) {
            continue;
        }

        std::optional<Type> t = condition_to_type(*cas->condition, current_table, current_pipeline_input);
        if(!t.has_value()) {
            report_error("Could not resolve type for cascade condition");
            continue;
        }

        auto type = infer_type(id.name.get(), current_table, current_pipeline_input);
        if (!type) {
            report_error("Could not resolve cascade receiver type");
            continue;
        }

        SymbolEntry* class_entry = current_table ? current_table->lookup_global(type->name) : nullptr;
        if(!class_entry || !class_entry->scope) {
            report_error("Could not resolve class scope for cascade receiver: " + type->name);
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

        const_cast<Condition*>(cas->condition.get())->accept(*this);

        if (cas->op == "=") {
            continue;
        }
    }

    table = saved_table;
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

    auto* saved_table = table;
    if (scope) {
        table = scope;
    }

    const_cast<Condition*>(match_node.input.get())->accept(*this);

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

        const_cast<Condition*>(case_item.pattern.get())->accept(*this);

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
                                local_type = condition_to_type(*var->init.value(), table, current_pipeline_input);
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

    table = saved_table;
}

void TypeCheckingVisitor::visit(YieldNode& yield_node) {
    if (yield_node.value) {
        auto yielded_type = condition_to_type(*yield_node.value, table, current_pipeline_input);
        if (!yielded_type) {
            report_error("Could not resolve yield value type");
            return;
        }

        yield_node.value->accept(*this);
    }
}

void TypeCheckingVisitor::visit(DotNode& dot) {
    auto* saved_table = table;
    auto saved_pipeline_input = current_pipeline_input;

    if (dot.left) {
        dot.left->accept(*this);
    }

    auto left_type = dot.left ? resolve_node_type(*dot.left, table, current_pipeline_input) : std::nullopt;
    if (left_type && table) {
        if (auto* entry = table->lookup_global(left_type->name); entry && entry->scope) {
            table = entry->scope.get();
        }
    }

    if (dot.right) {
        dot.right->accept(*this);
    }

    table = saved_table;
    current_pipeline_input = saved_pipeline_input;
}

void TypeCheckingVisitor::visit(PipelineNode& pipe) {
    auto saved_pipeline_input = current_pipeline_input;
    auto left_type = pipe.left ? infer_type(pipe.left.get(), table, saved_pipeline_input) : std::nullopt;
    if (!left_type) {
        report_error("Could not resolve pipeline input type");
        return;
    }

    if (!pipe.right) {
        report_error("Pipeline requires a right-hand expression");
        return;
    }

    if (!contains_placeholder(*pipe.right)) {
        report_error("Pipeline right-hand expression must contain placeholder _");
        return;
    }

    if (pipe.left) {
        pipe.left->accept(*this);
    }

    current_pipeline_input = left_type;
    auto right_type = condition_to_type(*pipe.right, table, *left_type);
    if (!right_type) {
        report_error("Could not resolve pipeline result type");
        current_pipeline_input = saved_pipeline_input;
        return;
    }

    pipe.right->accept(*this);
    current_pipeline_input = saved_pipeline_input;
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

void TypeCheckingVisitor::visit(ReturnNode& ret_node) {
    if (!current_return_type) {
        if (ret_node.ret) {
            ret_node.ret->accept(*this);
        }
        return;
    }

    if (!ret_node.ret) {
        if (current_return_type->type != TypeCategory::VOID) {
            report_error("Missing return value for function returning " + current_return_type->name);
        }
        return;
    }

    auto actual_type = infer_type(ret_node.ret.get(), table, current_pipeline_input);
    if (!actual_type) {
        report_error("Could not resolve return value type");
        return;
    }

    if (!same_type(*actual_type, *current_return_type) && !can_implicitly_convert(*actual_type, *current_return_type)) {
        report_error("Return type mismatch: expected " + current_return_type->name + ", got " + actual_type->name);
    }

    if (ret_node.ret) {
        ret_node.ret->accept(*this);
    }
}
