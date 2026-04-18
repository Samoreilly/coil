#include "compiler/middle-end/ir/IRGenerator.h"
#include "compiler/middle-end/ir/BasicBlock.h"

#include <iostream>

int main() {
    auto global = std::make_unique<GlobalNode>();
    auto fn = std::make_unique<FnNode>();
    fn->name = "smoke";
    fn->return_type = Type{TypeCategory::INT, 32, true, "int"};

    auto body = std::make_unique<BodyNode>();
    auto ret = std::make_unique<ReturnNode>();
    ret->ret = std::make_unique<IntegerCondition>(Token{TokenType::INTEGER_LITERAL, "1", "smoke", 1, 1});
    body->statements.push_back(std::move(ret));
    fn->body = std::move(body);

    global->globals.push_back(std::move(fn));

    ir::IRGenerator generator;
    auto module = generator.generate(*global);
    auto blocks = ir::blockify(module);

    if (module.functions.size() != 1 || blocks.functions.size() != 1) {
        std::cerr << "unexpected IR shape\n";
        return 1;
    }

    if (module.functions.front().quads.empty()) {
        std::cerr << "no quads emitted\n";
        return 1;
    }

    return 0;
}
