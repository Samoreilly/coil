#include "Lexer.h"
#include "Token.h"

void Lexer::lex(std::string con) {

    start = 0, end = 0, line = 1, col = 1, length = con.length();

    while(end < length) {
        
        char c = con[end];
        
        fmt::println(stderr, "Current char: {}", c);

        if(is_whitespace(c)) continue;     
        
        if(std::isalpha(c) || c == '_') {
            start = end;
            
            int start_col = col;
            while(end < length && (std::isalnum(con[end]) || con[end] == '_')) {
                end++;
                col++;
            }
            
            std::string_view data(con.data() + start, end - start);
            std::string ident(data);
            

            auto it = KEYWORDS.find(ident);
            
            if (it != KEYWORDS.end()) {
                tokens.push_back({it->second, ident, line, col});
            
            }else if(VISIBILITY.count(ident)){
                tokens.push_back({TokenType::VIS, ident, line, start_col}); 
            
            }else if(ident == "_"){
                tokens.push_back({TokenType::PLACEHOLDER, ident, line, start_col});
            
            }else if(ACCESS_MAP.count(ident)) {
                tokens.push_back({TokenType::ACCESS, ident, line, start_col});
            
            }else if(TYPES.count(ident)) {
                tokens.push_back({TokenType::TYPE_KEYWORD, ident, line, start_col});
            
            }else if(ident == "fn") {
                tokens.push_back({TokenType::FN, ident, line, start_col});
            
            }else if(ident == "crate") {
                tokens.push_back({TokenType::CRATE, ident, line, start_col});
            
            }else if(ident == "class") {
                tokens.push_back({TokenType::CLASS, ident, line, start_col});
            
            }else if(ident == "for") {
                tokens.push_back({TokenType::FOR, ident, line, start_col});
            
            }else if(ident == "if") {
                tokens.push_back({TokenType::IF, ident, line, start_col});
            
            }else if(ident == "while") {
                tokens.push_back({TokenType::WHILE, ident, line, start_col});
            
            }else if(ident == "return") {
                tokens.push_back({TokenType::RETURN, ident, line, start_col});
            
            }else if(RESERVED.count(ident)) {
                tokens.push_back({TokenType::KEYWORD, ident, line, start_col});
            
            }else {
                tokens.push_back({TokenType::IDENTIFIER, ident, line, start_col});
            }

            start = end;
            continue;
        }

        if(std::isdigit(c)) {
            start = end;

            int start_col = col;
            while(end < length && (std::isdigit(con[end]) || con[end] == '.')) {
                end++;
                col++;
            }

            std::string_view data(con.data() + start, end - start);
            std::string ident(data);
            
            if(data.find('.') != std::string::npos) {
                tokens.push_back({TokenType::DOUBLE_LITERAL, ident, line, start_col});
            }else {
                tokens.push_back({TokenType::INTEGER_LITERAL, ident, line, start_col});
            }

            start = end;
            continue;
        }


        if(is_symbol(c)) {
            start = end;
            
            if(c == '/' && con[end + 1] == '/') {

                while(con[end] != '\n') {
                    end++;
                    col++;
                }

                line++;
                col = 1;
                start = ++end;
                continue;

            }else if (c == '\"') {
                int start_col = col++;
                start = ++end;

                while (end < length && con[end] != '\"') {
                    if (con[end] == '\n') {
                        line++;
                        col = 0;
                    }
                    end++;
                    col++;
                }

                std::string data(con.data() + start, end - start);
                tokens.push_back({TokenType::STRING_LITERAL, data, line, start_col});

                col++;
                start = ++end;
                continue;
            
            }else if(c == '\'') {
                start = ++end;

                int start_col = col;
                while(end < length && con[end] != '\'') {
                    end++;
                    col++;
                }

                std::string data(con.data() + start, end - start);
                tokens.push_back({TokenType::CHAR_LITERAL, data, line, start_col});
                
                col++;
                start = ++end;
                continue;
            }
            
            fmt::println("\n\n\n========SYMBOL=======   {}\n\n\n", std::string(1, c));
            tokens.push_back({TokenType::SYMBOL, std::string(1, c), line, col});

            start = ++end;
            col++;
            continue;
        }

        if(is_operator(c)) {
            
            start = end;
            int start_col = col;
            col++;
            
            std::string op;
            
            switch(c) {
                
                case '-':
                case '+':
                    op += c;
                    end++;
                    
                    if(c == '-' && con[end] == '>') {
                        op += '>';
                        col++;
                        end++;
                        tokens.push_back({TokenType::ARROW, op, line, col});
                        continue;
                    }else if(con[end] == c) {
                        op += c;
                        col++;
                        end++;

                        tokens.push_back({TokenType::UNARY_OP, op, line, start_col});
                        continue;
                    }else if(con[end] == '=') {
                        op += '=';
                        col++;
                        end++;
                    }

                    tokens.push_back({TokenType::OPERATOR, op, line, start_col});
                    break;

                case '>':
                case '<':
                case '/':
                case '*':
                    op += c;    
                    end++;

                    if(con[end] == '=') {
                        op += '=';
                        col++;
                        end++;           
                    }
                        
                    tokens.push_back({TokenType::OPERATOR, op, line, start_col});
                    break;
                        
                case '=':
                    op += c;
                    end++;

                    if(con[end] == '=') {
                        op += '=';
                        col++;
                        end++;
                    }

                    tokens.push_back({TokenType::OPERATOR, op, line, start_col});
                    break; 

                case '!':
                    op += c;
                    end++;
                
                    tokens.push_back({TokenType::UNARY_OP, op, line, start_col});
                    break;

                case '|': {
                    op += c;
                    end++;
                    
                    if(con[end] == '>') {
                        op += '>';
                        col++;
                        end++;

                        tokens.push_back({TokenType::PIPELINE, op, line, start_col});
                        break;
                    }else if(con[end] == '|') {
                        op += '|';
                        col++;
                        end++;

                        tokens.push_back({TokenType::LOGICAL_OP, op, line, start_col});

                        break;
                    }else if(con[end] == '=') {
                        op += '=';
                        col++;
                        end++;

                        tokens.push_back({TokenType::BITWISE_OP, op, line, start_col});
                        
                        break;
                    }

                    tokens.push_back({TokenType::BITWISE_OP, std::string(1, c), line, start_col});
                    break;
                }
                case '&':
                    op += c;
                    end++;


                    if(con[end] == '&') {
                        op += '&';
                        col++;
                        end++;

                        tokens.push_back({TokenType::LOGICAL_OP, op, line, col});

                        break;
                    }else if(con[end] == '=') {
                        op += '=';
                        col++;
                        end++;

                        tokens.push_back({TokenType::BITWISE_OP, op, line, col});
                        break;
                    }

                    tokens.push_back({TokenType::BITWISE_OP, std::string(1, c), line, col});
                    break;

                case '^':
                    op += c;
                    end++;

                    if(con[end] == '=') {
                        op += '=';
                        col++;
                        end++;

                        tokens.push_back({TokenType::BITWISE_OP, op, line, col});
                        break;

                    }
                    
                    tokens.push_back({TokenType::BITWISE_OP, op, line, col});
                    break;

                case '~':
                    op += c;
                    end++;

                    if(con[end] == '=') {
                        op += '=';
                        col++;
                        end++;

                        tokens.push_back({TokenType::BITWISE_OP, op, line, col});
                        break;
                    }

                    tokens.push_back({TokenType::BITWISE_OP, std::string(1, c), line, col});
                    break;
            };

            start = end;
            continue;

        }
 
        if(end > start) {            
            std::string_view dat(con.data() + start, end - start);
            std::string data(dat);
            
            tokens.push_back({TokenType::IDENTIFIER, data, line, col});
            
            start = ++end;
            continue;
        }
    }
    tokens.push_back({TokenType::END_OF_FILE, "END_OF_FILE", line, col});
 
}
