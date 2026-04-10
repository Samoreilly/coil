#pragma once

#include "../../Support/Diagnostics/Diagnostics.h"
#include "../../ast/Node.h"
#include "../../ast/Condition.h"
#include "../lexer/Token.h"

#include <vector>
#include <memory>

class Parser final {

    std::unique_ptr<BodyNode>      parse_body();
    std::unique_ptr<Node>          parse_statement();

    std::unique_ptr<Condition>     parse_dot();
    std::unique_ptr<Condition>     parse_pipeline();
    std::unique_ptr<FnNode>        parse_fn(const std::string_view vis = "");
    std::unique_ptr<Condition>     parse_fn_call(const std::string_view vis = "");
    std::unique_ptr<ClassNode>     parse_class(const std::string_view = "");  
    std::unique_ptr<Condition>     parse_fn_call_with_name(const std::string& name, const std::string_view vis);
    std::unique_ptr<CrateNode>     parse_crate(const std::string_view vis = "");
    std::unique_ptr<IfNode>        parse_if();
    std::unique_ptr<ElseIfNode>    parse_elseif();
    std::unique_ptr<ElseNode>      parse_else();
    std::unique_ptr<WhileNode>     parse_while();
    std::unique_ptr<ForNode>       parse_for();
    std::unique_ptr<VariableNode>  parse_variable(const std::string_view vis = "", const std::string_view access = "");                 

    std::unique_ptr<Node>          parse_incr();
    std::unique_ptr<ReturnNode>    parse_return();

    std::unique_ptr<Condition>     parse_comparison();
    std::unique_ptr<Condition>     parse_add();
    std::unique_ptr<Condition>     parse_mul();
    std::unique_ptr<Condition>     parse_primary();

    Visibility handle_visibility(const std::string_view vis); 
    std::vector<Token> tokens;

    int index = 0, length = 0;

    Diagnostics& diagnostics;

    Token eof_token() const {
        if (length > 0) {
            const Token& last = tokens[length - 1];
            return {TokenType::END_OF_FILE, "END_OF_FILE", last.file_name, last.line, last.col};
        }

        return {TokenType::END_OF_FILE, "END_OF_FILE", "<unknown>", 1, 1};
    }

    void report_error(std::string message, const Token& token) {
        diagnostics.send(
            DiagPhase::PARSER,
            std::move(message),
            token.line,
            token.col,
            token.token_value,
            token.file_name);
    }

    void synchronize_statement() {
        while (index < length &&
               !check(TokenType::SYMBOL, ";") &&
               !check(TokenType::SYMBOL, "}") &&
               !check(TokenType::END_OF_FILE)) {
            advance();
        }

        if (check(TokenType::SYMBOL, ";")) {
            advance();
        }
    }
    /*
        if expected is provided,
        token[index].type == type
        && token[index].value = expected

        otherwise:
        token[index].type == type
    */

    bool consume(TokenType type, std::string_view expected = "") {
        Token curr = get_token();

        if (!expected.empty()) {
            if (curr.token_type == type && curr.token_value == expected) {
                index++;
                return true;
            }
        } else {
            if (curr.token_type == type) {
                index++;
                return true;
            }
        }

        std::string msg = "Token '" + curr.token_value + "' at line: "
            + std::to_string(curr.line)
            + " col: "
            + std::to_string(curr.col)
            + " did not match expected ";

        if (!expected.empty()) {
            msg += "value '";
            msg += expected;
            msg += "'";
        } else {
            msg += "type";
        }

        report_error(msg, curr);

        if (index < length && curr.token_type != TokenType::END_OF_FILE) {
            index++;
        }

        return false;
    }

    bool check(TokenType type, const std::string& expected = "") {
        if(index >= length) return false;
        if(tokens[index].token_type != type) return false;
        if(!expected.empty() && tokens[index].token_value != expected) return false;

        return true;
    }

    /*
        Returns next tokens and moves index forward
    */
    Token advance() {
        if(index + 1 < length) {
            index++;
            return tokens[index];
        }

        return eof_token();
    }

    Token get_token() {
        if (index >= length) {
            return eof_token();
        }

        return tokens[index];
    }

    Token peek_next(int n = 1) {
        if(index + n < length) {
            return tokens[index + n];
        }

        return eof_token();
    }

    Token peek_prev(int n = 1) {
        if(index - n >= 0) {
            return tokens[index - n];
        }

        return eof_token();

    }

public:

    Parser(std::vector<Token>&& t, Diagnostics& d) : diagnostics(d), tokens(std::move(t)), length(tokens.size()) {}

    std::unique_ptr<GlobalNode> construct_ast();
 
};
