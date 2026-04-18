
#include "compiler/frontend/lexer/Token.h"
#include "compiler/frontend/lexer/Lexer.h"
#include "compiler/io/FileHandler.h"
#include "compiler/frontend/parser/Parser.h"
#include "compiler/ast/visitors/PrintVisitor.h"
#include "compiler/Support/Diagnostics/Diagnostics.h"
#include "compiler/Support/ErrorHandling/Error.h"
#include "compiler/middle-end/semantic/ConversionVisitor.h"
#include "compiler/middle-end/semantic/RegisterVisitor.h"
#include "compiler/middle-end/semantic/TypeCheckingVisitor.h"
#include "compiler/middle-end/ir/IRGenerator.h"
#include "compiler/middle-end/ir/BasicBlock.h"
#include <fmt/core.h>

int main(int argc, char* argv[]) {
    
    try {

        Diagnostics diagnostics;

        FileHandler file_handler{argv, argc};
        std::vector<std::string> file_contents = file_handler.load_files(argv, argc);

        Lexer lex(diagnostics);

        for(const auto& file : file_contents) {
            lex.lex(file);
        }

        if (diagnostics.has_errors()) {
            throw CompilationError(diagnostics);
        }
 
        std::vector<Token>& tokens = lex.tokens;

        Parser p{std::move(tokens), diagnostics};
        auto node = p.construct_ast();

        if (diagnostics.has_errors()) {
            throw CompilationError(diagnostics);
        }
        
    
        Semantic::SymbolTable global_scope("global");

        PrintVisitor printer;
        RegisterVisitor register_sem{&global_scope, diagnostics};
        ConversionVisitor conversion_sem{&global_scope, diagnostics};
        TypeCheckingVisitor type_checker{&global_scope, diagnostics};
        ir::IRGenerator ir_generator;

        if (node) {
            node->accept(register_sem);
            node->accept(conversion_sem);
            node->accept(type_checker);
            auto ir_module = ir_generator.generate(*node);
            auto block_module = ir::blockify(ir_module);
            (void)block_module;
            node->accept(printer);
        }
   
        if(diagnostics.has_errors()) {
            throw CompilationError(diagnostics);
        }

        return 0;
    
    } catch (const std::exception& e) {
        fmt::println(stderr, "{}", e.what());
        return 1;
    }
}
