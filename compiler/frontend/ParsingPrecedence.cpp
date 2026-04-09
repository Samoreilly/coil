
#include "../common/Node.h"
#include "Parser.h"
#include <memory>
#include <fmt/core.h>

std::unique_ptr<Condition> Parser::parse_fn_call(const std::string_view vis) {
    std::string name = get_token().token_value;
    advance();
    return parse_fn_call_with_name(name, vis);
}

std::unique_ptr<Condition> Parser::parse_fn_call_with_name(const std::string& name, const std::string_view vis) {
    consume(TokenType::SYMBOL, "(");

    // Constructor definition: ClassName(type name, ...) { ... }
    // Only attempt if we can prove it starts as a type/name pair.
    if ((check(TokenType::TYPE_KEYWORD) || check(TokenType::IDENTIFIER)) &&
        peek_next(1).token_type == TokenType::IDENTIFIER) {
        bool is_constructor_signature = true;
        std::vector<std::unique_ptr<Parameter>> params;

        while (!check(TokenType::SYMBOL, ")")) {
            if (check(TokenType::SYMBOL, ",")) {
                consume(TokenType::SYMBOL, ",");
            }

            if (!check(TokenType::TYPE_KEYWORD) && !check(TokenType::IDENTIFIER)) {
                is_constructor_signature = false;
                break;
            }

            Token type_tok = get_token();
            advance();

            if (!check(TokenType::IDENTIFIER)) {
                is_constructor_signature = false;
                break;
            }

            auto param = std::make_unique<Parameter>();
            auto it = TYPES.find(type_tok.token_value);
            if (it != TYPES.end()) {
                param->type = it->second;
            } else {
                param->type = type_tok.token_value;
            }
            param->name = get_token().token_value;
            consume(TokenType::IDENTIFIER);

            params.push_back(std::move(param));

            if (!check(TokenType::SYMBOL, ")") && !check(TokenType::SYMBOL, ",")) {
                is_constructor_signature = false;
                break;
            }
        }

        if (is_constructor_signature && check(TokenType::SYMBOL, ")")) {
            consume(TokenType::SYMBOL, ")");
            if (check(TokenType::SYMBOL, "{")) {
                auto con = std::make_unique<ConstructorNode>();
                con->name = name;
                con->params = std::move(params);
                if (!vis.empty()) con->vis = handle_visibility(vis);
                con->body = parse_body();
                return con;
            }
        }
    }

    // Regular function call
    std::vector<std::unique_ptr<Condition>> args;
    while (!check(TokenType::SYMBOL, ")")) {
        args.push_back(parse_pipeline());
        if (check(TokenType::SYMBOL, ",")) advance();
    }
    consume(TokenType::SYMBOL, ")");

    auto call = std::make_unique<FnCallNode>();
    call->name = name;
    call->arguments = std::move(args);

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
    
    fmt::print(stderr, "\n\nCURRENT TOKEN PIPELINE{}\n\n", get_token().token_value);

    while(check(TokenType::PIPELINE, "|>")) {
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
        
    fmt::print(stderr, "Parse primary{}", curr.token_value);
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
            fmt::print(stderr, "ENTERED IDENTIFIER{}", get_token().token_value);
            if (peek_next(1).token_value == "(") {
                fmt::print(stderr, "\n\n==FUNCTION CALL");
                left = parse_fn_call();
            } else {
                advance();
                left = std::make_unique<IdentifierCondition>(curr);
            }
            break;
        }

        case TokenType::TYPE_KEYWORD: {
            advance();
            left = std::make_unique<IdentifierCondition>(curr);
            break;
        }
        
        default: {
            advance();
            return std::make_unique<StringCondition>(curr);
        }
    }
    
    while (check(TokenType::SYMBOL, ".")) {
        advance(); // consume '.'

        Token field = get_token();
        advance(); // consume field name

        std::unique_ptr<Condition> right;
        if (check(TokenType::SYMBOL, "(")) {
            // method call — parse_fn_call currently reads name from stream
            // so we need a version that accepts the name already consumed
            right = parse_fn_call_with_name(field.token_value, "");
        } else {
            right = std::make_unique<IdentifierCondition>(field);
        }

        auto node = std::make_unique<DotNode>();
        node->left  = std::move(left);
        node->right = std::move(right);
        left = std::move(node);
    }

    return left;
}
