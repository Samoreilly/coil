
#include "compiler/Token.h"
#include "compiler/frontend/Lexer.h"
#include "compiler/file_handler/File_handler.h"
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
        

    return 0;
}
