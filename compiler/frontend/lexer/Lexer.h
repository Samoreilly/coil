#pragma once


#include "../../Support/Diagnostics/Diagnostics.h"

#include "Token.h"
#include <fmt/core.h>
#include <vector>

class Lexer {

    int line = 1, col = 1, start = 0, end = 0, length = 0;
    
    Diagnostics& diagnostics;

public:
    
    Lexer(Diagnostics& d) : diagnostics(d) {}

    std::vector<Token> tokens;

    void lex(std::string);

    // UTILITY FUNCTIONS

    inline bool is_whitespace(char c) { 
        if(c == '\n') {
            line++;
            start = ++end;
            col = 1;
            return true;
        }else if(std::isspace(c)) {
            col++;
            start = ++end;
            return true;
        }

        return false;
    }


    static const bool is_symbol(char c) {
        return c == '{' || c == '}' || c == '[' || c == ']' || c == '(' || c == ')' || c == ';' || c == ':' || c == ',' || c == '.' || c == '\"';
    }

    static const bool is_operator(char c) {
        return c == '+' || c == '-' || c == '*' || c == '/' || c == '='
            || c == '<' || c == '>' || c == '!' || c == '|' || c == '&'
            || c == '~' || c == '^' || c == '!';
    }

    
};
