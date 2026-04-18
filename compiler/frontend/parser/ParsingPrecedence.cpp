
#include "../../ast/Node.h"
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

    std::vector<std::unique_ptr<Parameter>> params;

    // constructor definition: ClassName(type name, ...) { ... }
    // only attempt if we can prove it starts as a type and name pair.
    if ((check(TokenType::TYPE_KEYWORD) || check(TokenType::IDENTIFIER)) &&
        peek_next(1).token_type == TokenType::IDENTIFIER) {
        
        bool is_constructor_signature = true;
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

            auto param = std::make_unique<Parameter>("test");
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

    // regular function call
    std::vector<std::unique_ptr<Condition>> args;
    while (!check(TokenType::SYMBOL, ")") && !check(TokenType::END_OF_FILE)) {
        int start_index = index;
        args.push_back(parse_pipeline());
        if (check(TokenType::SYMBOL, ",")) advance();

        if (index == start_index) {
            advance();
        }
    }

    if (check(TokenType::END_OF_FILE)) {
        report_error("Expected ')' to close function call arguments", get_token());
        auto call = std::make_unique<FnCallNode>();
        call->name = name;
        call->arguments = std::move(args);
        return call;
    }

    consume(TokenType::SYMBOL, ")");

    //must be a constructor
    if(get_token().token_value == "{") {
        auto con = std::make_unique<ConstructorNode>();
        
        con->name = name;
        con->params = std::move(params);
        con->vis = handle_visibility(vis);
        con->body = parse_body();
        return con;
    }

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


std::unique_ptr<Condition> Parser::parse_cascade(std::unique_ptr<Condition> base) {
    auto cascade_node = std::make_unique<CascadeNode>();
    cascade_node->name = std::move(base);

    while (check(TokenType::CASCADE)) {
        consume(TokenType::CASCADE);

        Token member = get_token();
        if (member.token_type != TokenType::IDENTIFIER) {
            report_error("Expected member name after '..'", member);
            break;
        }
        consume(TokenType::IDENTIFIER);

        auto step = std::make_unique<CascadeStep>();
        step->var_name = member.token_value;

        if (check(TokenType::SYMBOL, "(")) {
            step->op = "call";
            step->condition = parse_fn_call_with_name(member.token_value, "");
        } else if (check(TokenType::OPERATOR) && assign_ops.count(get_token().token_value)) {
            step->op = get_token().token_value;
            advance();

            bool prev_suppress = suppress_cascade_in_primary;
            suppress_cascade_in_primary = true;
            step->condition = parse_comparison();
            suppress_cascade_in_primary = prev_suppress;
        } else {
            report_error("Expected assignment or call after cascade member", get_token());
            break;
        }

        cascade_node->cascades.push_back(std::move(step));

        if (check(TokenType::SYMBOL, ";") && peek_next(1).token_type == TokenType::CASCADE) {
            consume(TokenType::SYMBOL, ";");
        }
    }

    return cascade_node;
}


std::unique_ptr<Condition> Parser::parse_pipeline() {
    auto left = parse_comparison();

    if (check(TokenType::CASCADE)) {
        left = parse_cascade(std::move(left));
    }
    
    while(check(TokenType::PIPELINE, "|>")) {
        advance();

        auto right = parse_comparison();

        if (check(TokenType::CASCADE)) {
            right = parse_cascade(std::move(right));
        }

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

        case TokenType::MATCH: {
            left = parse_match();
            break;
        }

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
                    report_error("Invalid floating-point literal", curr);
                    break;
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
                    report_error("Invalid integer literal", curr);
                    break;
                }
            }
            advance();
            left = std::make_unique<IntegerCondition>(curr);
            break;
        }

        case TokenType::BOOL_LITERAL: {
            advance();
            if (curr.token_value != "true" && curr.token_value != "false") {
                report_error("Boolean literal must be 'true' or 'false'", curr);
            }
            left = std::make_unique<BoolCondition>(curr);
            break;
        }

        case TokenType::CHAR_LITERAL: {
            advance();
            if (curr.token_value.length() != 1) {
                report_error("Char literal must contain exactly one character", curr);
            }
            left = std::make_unique<CharCondition>(curr);
            break;
        }

        case TokenType::IDENTIFIER: {
            if (peek_next(1).token_value == "(") {
                left = parse_fn_call();
            
            }else {
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

        case TokenType::PLACEHOLDER: {
            advance();
            left = std::make_unique<IdentifierCondition>(curr);
            break;
        }
        
        default: {
            report_error("Unexpected token in expression: '" + curr.token_value + "'", curr);
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

    if (!suppress_cascade_in_primary && check(TokenType::CASCADE)) {
        left = parse_cascade(std::move(left));
    }

    return left;
}
