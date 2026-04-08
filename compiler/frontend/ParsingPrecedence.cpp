
#include "../common/Node.h"
#include "Parser.h"
#include <memory>

std::unique_ptr<Condition> Parser::parse_fn_call() {

    auto call = std::make_unique<FnCallNode>();

    call->name = get_token().token_value;
    advance();
    consume(TokenType::SYMBOL, "(");

    while(!check(TokenType::SYMBOL, ")")) {
        call->arguments.push_back(parse_comparison());

        if(check(TokenType::SYMBOL, ",")) {
            advance();
        }
    }

    consume(TokenType::SYMBOL, ")");
   
    return call;
}

std::unique_ptr<Condition> Parser::parse_comparison() {

    std::unique_ptr<Condition> left = parse_add();

    while(check(TokenType::OPERATOR, "<") || check(TokenType::OPERATOR, ">") || 
          check(TokenType::OPERATOR, "<=") || check(TokenType::OPERATOR, ">=") || 
          check(TokenType::OPERATOR, "==") || check(TokenType::OPERATOR, "!=")) {
        
        Token op = get_token();
        advance();

        auto right = parse_add();

        auto node = std::make_unique<BinaryExpression>();
        node->left = std::move(left);
        node->op = op.token_value;
        node->right = std::move(right);

        left = std::move(node);
    }

    return left;
}

std::unique_ptr<Condition> Parser::parse_pipeline() {
    auto left = parse_comparison();

    while(check(TokenType::SYMBOL, "|>")) {
        advance();

        auto right = parse_comparison();

        auto node = std::make_unique<PipelineNode>();
        node->left = std::move(left);
        node->right = std::move(right);

        left = std::move(node);
    }

    return left;
}
std::unique_ptr<Condition> Parser::parse_add() {

    std::unique_ptr<Condition> left = parse_mul();

    while(check(TokenType::OPERATOR, "+") || check(TokenType::OPERATOR, "-")) {
        Token op = get_token();
        advance();

        auto right = parse_mul();

        auto node = std::make_unique<BinaryExpression>();

        node->left = std::move(left);
        node->op = op.token_value;
        node->right = std::move(right);

        left = std::move(node);
    }

    return left;
}

std::unique_ptr<Condition> Parser::parse_mul() {

    std::unique_ptr<Condition> left = parse_primary();

    while(check(TokenType::OPERATOR, "*") || check(TokenType::OPERATOR, "/")) {
        Token op = get_token();
        advance();

        auto right = parse_primary();

        auto node = std::make_unique<BinaryExpression>();

        node->left = std::move(left);
        node->op = op.token_value;
        node->right = std::move(right);

        left = std::move(node);
    }

    return left;


}

std::unique_ptr<Condition> Parser::parse_primary() {
    Token curr = get_token();
    std::unique_ptr<Condition> left;

    switch (curr.token_type) {
        case TokenType::STRING_LITERAL: {
            advance();
            left = std::make_unique<StringCondition>(curr);
            break;
        }

        case TokenType::DOUBLE_LITERAL: {
            advance();
            bool found_dot{false};

            for (char c : curr.token_value) {
                if (!(std::isdigit(c) || c == '.') || (found_dot && c == '.')) {
                    throw std::runtime_error("Characters are not allowed in integers line"
                        + std::to_string(curr.line) + " col: "
                        + std::to_string(curr.col) + "\n");
                } else if (c == '.') {
                    found_dot = true;
                }
            }
            left = std::make_unique<DoubleCondition>(curr);
            break;
        }

        case TokenType::INTEGER_LITERAL: {
            for (char c : curr.token_value) {
                if (curr.token_value.find('.') != std::string::npos || std::isalpha(c)) {
                    throw std::runtime_error("\nDecimal points or characters are not allowed in integers line: "
                        + std::to_string(curr.line) + " col: "
                        + std::to_string(curr.col) + "\n");
                }
            }
            advance();
            left = std::make_unique<IntegerCondition>(curr);
            break;
        }

        case TokenType::BOOL_LITERAL: {
            advance();
            if (curr.token_value != "true" && curr.token_value != "false") {
                throw std::runtime_error("Boolean values can only be true or false"
                    + std::to_string(curr.line) + " col: "
                    + std::to_string(curr.col) + "\n");
            }
            left = std::make_unique<BoolCondition>(curr);
            break;
        }

        case TokenType::CHAR_LITERAL: {
            advance();
            if (curr.token_value.length() != 1) {
                throw std::runtime_error("Char must only be 1 character Line: "
                    + std::to_string(curr.line) + " col: "
                    + std::to_string(curr.col) + "\n");
            }
            left = std::make_unique<CharCondition>(curr);
            break;
        }

        case TokenType::IDENTIFIER: {
            if (peek_next(1).token_value == "(") {
                left = parse_fn_call();
            } else {
                advance();
                left = std::make_unique<IdentifierCondition>(curr);
            }
            break;
        }

        default: {
            advance();
            return std::make_unique<StringCondition>(curr);
        }
    }

    while (check(TokenType::SYMBOL, ".")) {
        advance();
        auto right = parse_primary();
        auto node = std::make_unique<DotNode>();

        node->left = std::move(left);
        node->right = std::move(right);
        left = std::move(node);
    }

    return left;
}
