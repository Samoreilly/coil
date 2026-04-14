#include "TypeCheckingVisitor.h"


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

// std::unique_ptr<Parameter> argument_to_parameter(const Condition& argument, SymbolTable* table) {
//     if (dynamic_cast<const IntegerCondition*>(&argument)) {
//         return std::make_unique<Parameter>(TYPES.at("int"));
//     }

//     if (dynamic_cast<const DoubleCondition*>(&argument)) {
//         return std::make_unique<Parameter>(TYPES.at("f64"));
//     }

//     if (dynamic_cast<const BoolCondition*>(&argument)) {
//         return std::make_unique<Parameter>(TYPES.at("bool"));
//     }

//     if (dynamic_cast<const StringCondition*>(&argument)) {
//         return std::make_unique<Parameter>(TYPES.at("string"));
//     }

//     if (dynamic_cast<const CharCondition*>(&argument)) {
//         return std::make_unique<Parameter>(TYPES.at("char"));
//     }

//     if (const auto* ident = dynamic_cast<const IdentifierCondition*>(&argument)) {
//         if (table) {
//             if (auto* entry = table->lookup_global(ident->token.token_value)) {
//                 if (entry->type) {
//                     return std::make_unique<Parameter>(*entry->type);
//                 }
//             }
//         }
//     }

//     return std::make_unique<Parameter>(std::string{});
// }

} // namespace

// std::unique_ptr<ConstructorNode> TypeCheckingVisitor::promoteFnCallToConstructor(FnCallNode& call) {
//     auto constructor = std::make_unique<ConstructorNode>();
//     constructor->name = call.name;

//     for (auto& argument : call.arguments) {
//         if (!argument) {
//             continue;
//         }

//         constructor->params.push_back(argument_to_parameter(*argument, table));
//     }

//     call.arguments.clear();

//     return constructor;
// }

void TypeCheckingVisitor::visit(GlobalNode& global) {

    for(const auto& node : global.globals) {
        if(node) node->accept(*this);
    }

}

void TypeCheckingVisitor::visit(VariableNode& v) {

}

void TypeCheckingVisitor::visit(BinaryExpression& b) {

}

void TypeCheckingVisitor::visit(FnNode& b) {

}

void TypeCheckingVisitor::visit(FnCallNode& b) {
    SymbolEntry* parent = table->lookup_local(b.name);
    
    //constructor node indistingishable from function call in parsing
    if(parent && parent->node_kind == NodeKind::CLASS_NODE) {
        report_error("Function name conflicts with class name: " + b.name);    
    }
}

void TypeCheckingVisitor::visit(ConstructorNode& c) {

}

void TypeCheckingVisitor::visit(BodyNode& b) {

}

void TypeCheckingVisitor::visit(ClassNode& c) {

}

void TypeCheckingVisitor::visit(CrateNode& cr) {

}

void TypeCheckingVisitor::visit(WhileNode& b) {

}

void TypeCheckingVisitor::visit(ForNode& b) {

}

void TypeCheckingVisitor::visit(IfNode& b) {

}

void TypeCheckingVisitor::visit(ElseIfNode& b) {

}

void TypeCheckingVisitor::visit(ElseNode& b) {

}

void TypeCheckingVisitor::visit(MatchNode& b) {

}

void TypeCheckingVisitor::visit(UnaryIncrNode& b) {

}

void TypeCheckingVisitor::visit(DotNode& b) {

}

void TypeCheckingVisitor::visit(CascadeNode& id) {

}

void TypeCheckingVisitor::visit(PipelineNode& id) {

}

void TypeCheckingVisitor::visit(ReturnNode& b) {

}

void TypeCheckingVisitor::visit(IdentifierCondition& b) {

}

void TypeCheckingVisitor::visit(IntegerCondition& id) {

}

void TypeCheckingVisitor::visit(DoubleCondition& id) {

}

void TypeCheckingVisitor::visit(BoolCondition& id) {

}

void TypeCheckingVisitor::visit(StringCondition& id) {

}

void TypeCheckingVisitor::visit(CharCondition& id) {

}
