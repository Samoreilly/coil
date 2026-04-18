#include "compiler/frontend/lexer/Lexer.h"
#include "compiler/frontend/parser/Parser.h"
#include "compiler/middle-end/ir/IRGenerator.h"
#include "compiler/middle-end/ir/BasicBlock.h"
#include "compiler/Support/Diagnostics/Diagnostics.h"

#include <iostream>

int main() {
    Diagnostics diagnostics;
    Lexer lexer(diagnostics);
    lexer.lex("fn smoke() { return 1; }");

    Parser parser(std::move(lexer.tokens), diagnostics);
    auto ast = parser.construct_ast();

    if (diagnostics.has_errors() || !ast) {
        std::cerr << "parse failed\n";
        return 1;
    }

    ir::IRGenerator generator;
    auto module = generator.generate(*ast);
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
