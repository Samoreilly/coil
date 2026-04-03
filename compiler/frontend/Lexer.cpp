#include "Lexer.h"
#include "../Token.h"

void Lexer::lex(std::string con) {

    start = 0, end = 0, line = 1, col = 1, length = con.length();

    while(end < length) {
        
        char c = con[end];
        
        fmt::println(stderr, "Current char: {}", c);

        is_whitespace(c);      
        
        if(std::isalpha(c)) {
            start = end;
            
            while(end < length && (std::isalpha(con[end]) || con[end] == '_')) {
                end++;
                col++;
            }
            
            std::string_view data(con.data() + start, end - start);
            std::string ident(data);

            if(RESERVED.count(ident)) {
                tokens.push_back({TokenType::KEYWORD, ident, line, col});
            
            }else if(TYPES.count(ident)) {
                tokens.push_back({TYPES.at(ident), ident, line, col});
            }else {
                tokens.push_back({TokenType::IDENTIFIER, ident, line, col});
            }

            start = end;
            continue;
        }

        if(std::isdigit(c)) {
            start = end;

            while(end < length && (std::isdigit(con[end]) || con[end] == '.')) {
                end++;
                col++;
            }

            std::string_view data(con.data() + start, end - start);
            std::string ident(data);
            
            if(data.find('.') != std::string::npos) {
                tokens.push_back({TokenType::DOUBLE, ident, line, col});
            }else {
                tokens.push_back({TokenType::INTEGER, ident, line, col});
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
                continue;

            }else if (c == '\"') {
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
                tokens.push_back({TokenType::STRING, data, line, col});

                col++;
                start = ++end;
                continue;
            
            }else if(c == '\'') {
                start = ++end;

                while(end < length && con[end] != '\'') {
                    end++;
                    col++;
                }

                std::string data(con.data() + start, end - start);
                tokens.push_back({TokenType::CHAR, data, line, col});
                
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
            col++;

            std::string op;
            
            switch(c) {
                
                case '-':
                case '+':
                    op += c;
                    end++;

                    if(con[end] == c) {
                        op += c;
                        col++;
                        end++;

                        tokens.push_back({TokenType::UNARY_OP, op, line, col});
                        continue;
                    }else if(con[end] == '=') {
                        op += '=';
                        col++;
                        end++;
                    }

                    tokens.push_back({TokenType::OPERATOR, op, line, col});
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
                        
                    tokens.push_back({TokenType::OPERATOR, op, line, col});
                    break;
                        
                case '=':
                    op += c;
                    end++;

                    if(con[end] == '=') {
                        op += '=';
                        col++;
                        end++;
                    }

                    tokens.push_back({TokenType::OPERATOR, op, line, col});
                    break; 

                case '!':
                    op += c;
                    end++;
                
                    tokens.push_back({TokenType::UNARY_OP, op, line, col});
                    break;

                case '|':
                    op += c;
                    end++;

                    if(con[end] == '>') {
                        op += '>';
                        col++;
                        end++;

                        tokens.push_back({TokenType::PIPELINE, op, line, col});
                        break;
                    }else if(con[end] == '|') {
                        op += '|';
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

                case '&':

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

            start = ++end;
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


