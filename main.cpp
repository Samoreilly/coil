
#include "compiler/frontend/lexer/Token.h"
#include "compiler/frontend/lexer/Lexer.h"
#include "compiler/io/FileHandler.h"
#include "compiler/frontend/parser/Parser.h"
#include "compiler/ast/visitors/PrintVisitor.h"
#include <fmt/core.h>

int main(int argc, char* argv[]) {
     
    fmt::println("Coil-lang");

    FileHandler file_handler{argv, argc};
    std::vector<std::string> file_contents = file_handler.load_files(argv, argc);

    fmt::println("Test1");
    Lexer lex;

    for(const auto& file : file_contents) {
        lex.lex(file);
    }
 
    fmt::println("Test2");   
    std::vector<Token>& tokens = lex.tokens;
    for(const auto& t : tokens) {
        fmt::println("\nTokenType: {}\nToken-value: {}\nLine: {}\nColumn: {}", tokenTypeToString.at(t.token_type), t.token_value, t.line, t.col);
    }

    Parser p{std::move(tokens)};
    auto node = p.construct_ast();
    
    PrintVisitor v;
    
    if (node) node->accept(v);

    return 0;
}
