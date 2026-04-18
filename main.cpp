
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
#include "compiler/middle-end/ir/IRPrint.h"
#include <fmt/core.h>

int main(int argc, char* argv[]) {
    
    try {

        Diagnostics diagnostics;
        bool print_ir_flag = false;
        std::vector<char*> filtered_argv;
        filtered_argv.reserve(static_cast<std::size_t>(argc));
        filtered_argv.push_back(argv[0]);

        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "--ir") {
                print_ir_flag = true;
                continue;
            }

            filtered_argv.push_back(argv[i]);
        }

        int filtered_argc = static_cast<int>(filtered_argv.size());

        FileHandler file_handler{filtered_argv.data(), filtered_argc};
        std::vector<std::string> file_contents = file_handler.load_files(filtered_argv.data(), filtered_argc);

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
            if (print_ir_flag) {
                fmt::print("{}", ir::print(block_module));
            } else {
                node->accept(printer);
            }
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
