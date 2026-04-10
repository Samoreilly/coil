#include "Lexer.h"
#include "Token.h"

void Lexer::lex(std::string con) {

    start = 0, end = 0, line = 1, col = 1, length = con.length();
    current_file_name = "<unknown>";

    if (length >= 8 && con.rfind("[[FILE:", 0) == 0) {
        size_t marker_end = con.find("]]", 7);
        if (marker_end != std::string::npos) {
            current_file_name = con.substr(7, marker_end - 7);
            start = static_cast<int>(marker_end + 2);
            end = static_cast<int>(marker_end + 2);
        }
    }

    auto push_token = [this](TokenType type, std::string value, int line_value, int col_value) {
        tokens.push_back(Token{type, std::move(value), current_file_name, line_value, col_value});
    };

    while(end < length) {
        
        char c = con[end];
        
        // fmt::println(stderr, "Current char: {}", c);

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
                push_token(it->second, ident, line, col);
            
            }else if(VISIBILITY.count(ident)){
                push_token(TokenType::VIS, ident, line, start_col);
            
            }else if(ident == "_"){
                push_token(TokenType::PLACEHOLDER, ident, line, start_col);
            
            }else if(ACCESS_MAP.count(ident)) {
                push_token(TokenType::ACCESS, ident, line, start_col);
            
            }else if(TYPES.count(ident)) {
                push_token(TokenType::TYPE_KEYWORD, ident, line, start_col);
            
            }else if(ident == "fn") {
                push_token(TokenType::FN, ident, line, start_col);
            
            }else if(ident == "crate") {
                push_token(TokenType::CRATE, ident, line, start_col);
            
            }else if(ident == "class") {
                push_token(TokenType::CLASS, ident, line, start_col);
            
            }else if(ident == "for") {
                push_token(TokenType::FOR, ident, line, start_col);
            
            }else if(ident == "if") {
                push_token(TokenType::IF, ident, line, start_col);
            
            }else if(ident == "while") {
                push_token(TokenType::WHILE, ident, line, start_col);
            
            }else if(ident == "return") {
                push_token(TokenType::RETURN, ident, line, start_col);
            
            }else if(RESERVED.count(ident)) {
                push_token(TokenType::KEYWORD, ident, line, start_col);
            
            }else {
                push_token(TokenType::IDENTIFIER, ident, line, start_col);
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
                push_token(TokenType::DOUBLE_LITERAL, ident, line, start_col);
            }else {
                push_token(TokenType::INTEGER_LITERAL, ident, line, start_col);
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
                push_token(TokenType::STRING_LITERAL, data, line, start_col);

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
                push_token(TokenType::CHAR_LITERAL, data, line, start_col);
                
                col++;
                start = ++end;
                continue;
            }
            
            //fmt::println("\n\n\n========SYMBOL=======   {}\n\n\n", std::string(1, c));
            push_token(TokenType::SYMBOL, std::string(1, c), line, col);

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
                        push_token(TokenType::ARROW, op, line, col);
                        continue;
                    }else if(con[end] == c) {
                        op += c;
                        col++;
                        end++;

                        push_token(TokenType::UNARY_OP, op, line, start_col);
                        continue;
                    }else if(con[end] == '=') {
                        op += '=';
                        col++;
                        end++;
                    }

                    push_token(TokenType::OPERATOR, op, line, start_col);
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
                        
                    push_token(TokenType::OPERATOR, op, line, start_col);
                    break;
                        
                case '=':
                    op += c;
                    end++;

                    if(con[end] == '=') {
                        op += '=';
                        col++;
                        end++;
                    }

                    push_token(TokenType::OPERATOR, op, line, start_col);
                    break; 

                case '!':
                    op += c;
                    end++;
                
                    push_token(TokenType::UNARY_OP, op, line, start_col);
                    break;

                case '|': {
                    op += c;
                    end++;
                    
                    if(con[end] == '>') {
                        op += '>';
                        col++;
                        end++;

                        push_token(TokenType::PIPELINE, op, line, start_col);
                        break;
                    }else if(con[end] == '|') {
                        op += '|';
                        col++;
                        end++;

                        push_token(TokenType::LOGICAL_OP, op, line, start_col);

                        break;
                    }else if(con[end] == '=') {
                        op += '=';
                        col++;
                        end++;

                        push_token(TokenType::BITWISE_OP, op, line, start_col);
                        
                        break;
                    }

                    push_token(TokenType::BITWISE_OP, std::string(1, c), line, start_col);
                    break;
                }
                case '&':
                    op += c;
                    end++;


                    if(con[end] == '&') {
                        op += '&';
                        col++;
                        end++;

                        push_token(TokenType::LOGICAL_OP, op, line, col);

                        break;
                    }else if(con[end] == '=') {
                        op += '=';
                        col++;
                        end++;

                        push_token(TokenType::BITWISE_OP, op, line, col);
                        break;
                    }

                    push_token(TokenType::BITWISE_OP, std::string(1, c), line, col);
                    break;

                case '^':
                    op += c;
                    end++;

                    if(con[end] == '=') {
                        op += '=';
                        col++;
                        end++;

                        push_token(TokenType::BITWISE_OP, op, line, col);
                        break;

                    }
                    
                    push_token(TokenType::BITWISE_OP, op, line, col);
                    break;

                case '~':
                    op += c;
                    end++;

                    if(con[end] == '=') {
                        op += '=';
                        col++;
                        end++;

                        push_token(TokenType::BITWISE_OP, op, line, col);
                        break;
                    }

                    push_token(TokenType::BITWISE_OP, std::string(1, c), line, col);
                    break;
            };

            start = end;
            continue;

        }
 
        if(end > start) {            
            std::string_view dat(con.data() + start, end - start);
            std::string data(dat);
            
            push_token(TokenType::IDENTIFIER, data, line, col);
            
            start = ++end;
            continue;
        }
    }

    push_token(TokenType::END_OF_FILE, "END_OF_FILE", line, col);
  
}
