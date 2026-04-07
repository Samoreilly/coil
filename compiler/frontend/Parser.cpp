
#include "../common/Node.h"
#include "Parser.h"



std::unique_ptr<GlobalNode> Parser::construct_ast() {


    auto root_node = std::make_unique<GlobalNode>();

    for(const auto& node : tokens) {
        root_node->globals.push_back(parse_statement());
    }

    return std::move(root_node);

}


std::unique_ptr<Node> Parser::parse_statement() {
    Token curr = get_token();

    switch(curr.token_type) {

        case TokenType::FN: break;
        case TokenType::CRATE: break;
        

        //variable or function
        case TokenType::VIS: {
            
            advance();

            if(check(TokenType::FN, "fn")) {
                
                //passing visibility;
                parse_fn(curr.token_value);

            }else if(check(TokenType::TYPE_KEYWORD)) {
                //passing visibility;
                parse_variable(curr.token_value);
            }else {
                //error
            }

            break;
        }

        case TokenType::ACCESS: break;

        case TokenType::KEYWORD: break;
        case TokenType::DO: break;
        case TokenType::END: break;

        case TokenType::TYPE_KEYWORD: break;
        case TokenType::STRING_LITERAL: break;
        case TokenType::BOOL_LITERAL: break;
        case TokenType::DOUBLE_LITERAL: break;
        case TokenType::INTEGER_LITERAL: break;
        case TokenType::CHAR_LITERAL: break;

        case TokenType::IDENTIFIER: break;
        case TokenType::RETURN: break;
        case TokenType::CONSTRUCTOR: break;

        case TokenType::FOR: break;
        case TokenType::FOREACH: break;
        case TokenType::WHILE: break;
        case TokenType::IF: break;
        case TokenType::ELSE: break;
        case TokenType::ELSEIF: break;
        case TokenType::MATCH: break;

        case TokenType::OPERATOR: break;
        case TokenType::SYMBOL: break;
        case TokenType::UNARY_OP: break;
        case TokenType::LOGICAL_OP: break;
        case TokenType::BITWISE_OP: break;

        case TokenType::CASCADE: break;
        case TokenType::PIPELINE: break;

        case TokenType::END_OF_FILE: break;

        default: break;
    };
}

Visibility handle_visibility(const std::string_view vis) {
    if(to_vis_enum.find(vis) != to_vis_enum.end()) {
        return to_vis_enum.at(vis);
    
    }else [[unlikely]] {
        return Visibility::PUBLIC;
    }
}


std::unique_ptr<FnNode> Parser::parse_fn(const std::string_view vis) {

    auto fn = std::make_unique<FnNode>();

    fn->vis = handle_visibility(vis);
    
    consume(TokenType::FN, "fn");
    fn->name = get_token().token_value;

    consume(TokenType::SYMBOL, "(");

    //parameters
    while(!check(TokenType::SYMBOL, ")")) {
        
        if(check(TokenType::SYMBOL, ",")) {
            consume(TokenType::SYMBOL); 
        }

        auto param = std::make_unique<Parameter>();
        
        Token t = get_token();
     
        if(TYPES.find(t.token_value) != TYPES.end()) {
            param->type = TYPES.at(t.token_value);
        }else {
            //could be an object such as crate or class, this is a guess and will be checked in semantic analysis
            param->type = t.token_value;
        }

        param->name = advance().token_value;

        fn->parameters.push_back(std::move(param));
        advance();        
    }

    consume(TokenType::SYMBOL, ")");
    
    if(check(TokenType::ARROW, "->")) {
        consume(TokenType::ARROW, "->");
        //return type
        Token ret = get_token();
        
        if(TYPES.find(ret.token_value) != TYPES.end()) {
            //error
        }

        fn->return_type = TYPES.at(ret.token_value);
    }

    fn->body = parse_body();

    return fn;

}

std::unique_ptr<CrateNode> Parser::parse_crate() {

    auto crate = std::make_unique<CrateNode>();

    if(check(TokenType::VIS)) {
        crate->vis = handle_visibility(get_token().token_value);
    }

    crate->name = advance().token_value;
    
    do {

        crate->crate_vars.push_back(parse_variable());


    }while(check(TokenType::SYMBOL, "}"));

    consume(TokenType::SYMBOL, "}");
    consume(TokenType::SYMBOL, ";");

    return crate;

}

std::unique_ptr<Condition> Parser::parse_fn_call() {

}

std::unique_ptr<IfNode> Parser::parse_if() {

    auto i = std::make_unique<IfNode>();
    consume(TokenType::IF, "if"); 
    consume(TokenType::SYMBOL, "(");

}

std::unique_ptr<WhileNode> parse_while() {

}

std::unique_ptr<ForNode> parse_for() {

}

std::unique_ptr<Condition> parse_condition() {

}

std::unique_ptr<VariableNode> Parser::parse_variable(const std::string_view vis) {

}

std::unique_ptr<Node> parse_incr() {

}

std::unique_ptr<ReturnNode> parse_return() {

}

std::unique_ptr<Condition> parse_comparison() {

}

std::unique_ptr<Condition> parse_add() {

}

std::unique_ptr<Condition> parse_mul() {

}

std::unique_ptr<Condition> parse_primary() {

}


