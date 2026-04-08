
#include "../common/Node.h"
#include "Parser.h"
#include <fmt/core.h>


std::unique_ptr<GlobalNode> Parser::construct_ast() {


    auto root_node = std::make_unique<GlobalNode>();

    while(index < length && !check(TokenType::END_OF_FILE)) {
        root_node->globals.push_back(parse_statement());
    }

    return std::move(root_node);

}


std::unique_ptr<Node> Parser::parse_statement() {
    Token curr = get_token();

    switch(curr.token_type) {

        case TokenType::FN: return parse_fn();
        case TokenType::CRATE: return parse_crate();
        

        //variable or function
        case TokenType::VIS: {
            
            advance();

            if(check(TokenType::FN, "fn")) {
                
                //passing visibility;
                return parse_fn(curr.token_value);
            }else if(check(TokenType::ACCESS)){
                return parse_variable(curr.token_value, get_token().token_value);
            }else if(check(TokenType::CRATE)) {
                return parse_crate(curr.token_value);
            
            }else if(check(TokenType::TYPE_KEYWORD)) {
                //passing visibility;
                return parse_variable(curr.token_value);
            }else {
                //error
            }

            break;
        }

        case TokenType::ACCESS: {
            return parse_variable("", curr.token_value);
        }

        case TokenType::KEYWORD: break;

        case TokenType::TYPE_KEYWORD: return parse_variable();
        case TokenType::STRING_LITERAL: break;
        case TokenType::BOOL_LITERAL: break;
        case TokenType::DOUBLE_LITERAL: break;
        case TokenType::INTEGER_LITERAL: break;
        case TokenType::CHAR_LITERAL: break;

        case TokenType::IDENTIFIER: return parse_variable();
        case TokenType::RETURN: return parse_return();
        case TokenType::CONSTRUCTOR: break;

        case TokenType::FOR: return parse_for();
        case TokenType::FOREACH: break;
        case TokenType::WHILE: return parse_while();
        case TokenType::IF: return parse_if();
        case TokenType::ELSE: break;
        case TokenType::ELSEIF: break;
        case TokenType::MATCH: break;

        case TokenType::OPERATOR: break;
        case TokenType::SYMBOL: break;
        case TokenType::UNARY_OP: break;
        case TokenType::LOGICAL_OP: break;
        case TokenType::BITWISE_OP: break;

        case TokenType::CASCADE: break;
        case TokenType::PIPELINE: {
            return parse_pipeline();
            
        }

        case TokenType::END_OF_FILE: break;

        default: 
            throw std::runtime_error("Unexpected token in statement: " + curr.token_value);
    };
    return nullptr;
}

Visibility handle_visibility(const std::string_view vis) {
    auto it = to_vis_enum.find(vis);
    if (it != to_vis_enum.end()) {
        return it->second;
    } else {
        throw std::runtime_error("Unknown visibility in to_vis_enum: '" + std::string(vis) + "'");
    }
}


std::unique_ptr<FnNode> Parser::parse_fn(const std::string_view vis) {
    
    fmt::print(stderr, "\n\nENTERED FN\n\n");
    auto fn = std::make_unique<FnNode>();

    fn->vis = handle_visibility(vis);
    
    consume(TokenType::FN, "fn");
    fn->name = get_token().token_value;
    advance();

    consume(TokenType::SYMBOL, "(");

    //parameters
    while(!check(TokenType::SYMBOL, ")")) {
        
        if(check(TokenType::SYMBOL, ",")) {
            consume(TokenType::SYMBOL); 
        }

        auto param = std::make_unique<Parameter>();
        
        Token t = get_token();
     
        auto it = TYPES.find(t.token_value);
        if (it != TYPES.end()) {
            param->type = it->second;
        } else {
            // could be an object such as crate or class
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
        advance();
        
        if(TYPES.find(ret.token_value) == TYPES.end()) {
            throw std::runtime_error("Unknown return type: " + ret.token_value);
        }

        auto it = TYPES.find(ret.token_value);
        if (it != TYPES.end()) {
            fn->return_type = it->second;
        } else {
             throw std::runtime_error("Unknown return type in TYPES lookup: '" + ret.token_value + "'");
        }
    }

    fn->body = parse_body();

    return fn;

}

std::unique_ptr<CrateNode> Parser::parse_crate(const std::string_view vis) {

    fmt::print(stderr, "\n===============ENTERED CRATE==========\n");
    auto crate = std::make_unique<CrateNode>();

    if(!vis.empty()) {
        crate->vis = handle_visibility(vis);
    }
    
    consume(TokenType::CRATE, "crate");

    crate->name = get_token().token_value;
    advance();

    consume(TokenType::SYMBOL, "{");

    while(!check(TokenType::SYMBOL, "}")) {
        crate->crate_vars.push_back(parse_variable());
    }

    consume(TokenType::SYMBOL, "}");
    consume(TokenType::SYMBOL, ";");

    return crate;

}

std::unique_ptr<IfNode> Parser::parse_if() {

    auto i = std::make_unique<IfNode>();
    consume(TokenType::IF, "if"); 
    consume(TokenType::SYMBOL, "(");

    i->cond = parse_pipeline(); 
    consume(TokenType::SYMBOL, ")");

    i->body = parse_body();

    while(check(TokenType::ELSEIF)) {
        i->else_ifs.push_back(parse_elseif());
    }

    if(check(TokenType::ELSE)) {
        i->else_node = parse_else();
    }

    return i;
}

std::unique_ptr<ElseIfNode> Parser::parse_elseif() {
    auto ei = std::make_unique<ElseIfNode>();
    consume(TokenType::ELSEIF, "elseif");
    consume(TokenType::SYMBOL, "(");
    ei->cond = parse_pipeline();
    consume(TokenType::SYMBOL, ")");
    ei->body = parse_body();
    return ei;
}

std::unique_ptr<ElseNode> Parser::parse_else() {
    auto e = std::make_unique<ElseNode>();
    consume(TokenType::ELSE, "else");
    e->body = parse_body();
    return e;
}

std::unique_ptr<WhileNode> Parser::parse_while() {

    auto wh = std::make_unique<WhileNode>();

    consume(TokenType::WHILE, "while");
    consume(TokenType::SYMBOL, "(");

    wh->cond = parse_pipeline();
    consume(TokenType::SYMBOL, ")");

    wh->body = parse_body();

    return wh;
}

std::unique_ptr<ForNode> Parser::parse_for() {

    auto for_node = std::make_unique<ForNode>();

    consume(TokenType::FOR, "for");
    consume(TokenType::SYMBOL, "(");
    
    //for(x: int = 0;i < 10;i++) {}

    if(check(TokenType::SYMBOL, ";")) {
        consume(TokenType::SYMBOL, ";");
    }else {
        for_node->init = parse_variable();
    }


    if(!check(TokenType::SYMBOL, ";")) {
        for_node->cond = parse_pipeline();
    }

    consume(TokenType::SYMBOL, ";");

    if(!check(TokenType::SYMBOL, ")")) {
        for_node->incr = parse_incr();
    }
    
    consume(TokenType::SYMBOL, ")");
     

    for_node->for_body = parse_body();

    return for_node;
}


//parameters are passed from parse_statement()
std::unique_ptr<VariableNode> Parser::parse_variable(const std::string_view vis, const std::string_view access) {

    fmt::print(stderr, "\n\nEntered variable {} : {}\n\n", get_token().token_value, vis);
    auto var = std::make_unique<VariableNode>();
        
    //public or private
    if(!vis.empty()) {
        fmt::print(stderr, "\n================Visibility {}\n\n", std::string(vis));
        auto it = to_vis_enum.find(vis);
        if (it != to_vis_enum.end()) {
            var->vis = it->second;
        } else {
            throw std::runtime_error("Unknown visibility in VariableNode lookup: '" + std::string(vis) + "'");
        }
    }

    //access immutable or mutable
    if(!access.empty()) {
        fmt::print("\n\nENTERED ACCESS IF {}", get_token().token_value);
        
        if(access == "immut") {
            var->access = ACCESS::IMMUTABLE;
        }else{ 
            var->access = ACCESS::MUTABLE;
        }
        consume(TokenType::ACCESS);
    }    
    

    if(check(TokenType::TYPE_KEYWORD)) {
        auto it = TYPES.find(get_token().token_value);
        if (it != TYPES.end()) {
            var->type = it->second;
        } else {
            throw std::runtime_error("Unknown type in TYPES (VariableNode): '" + get_token().token_value + "'");
        }
        consume(TokenType::TYPE_KEYWORD);
    }


    Token name_tok = get_token();
    advance();
    std::unique_ptr<Condition> name_expr = std::make_unique<IdentifierCondition>(name_tok); 


    fmt::println(stderr, "CURRTOKEN PARSEVAR{}", get_token().token_value);
    //handles my_struct.name.num
    while (index < length && check(TokenType::SYMBOL, ".")) {
        advance();

        auto field = parse_primary();
        auto dot = std::make_unique<DotNode>();

        dot->left = std::move(name_expr);
        dot->right = std::move(field);
        name_expr = std::move(dot);
    }

    var->name = std::move(name_expr);

    //prior declaration, now initializing e.g. x = 10;
    if(check(TokenType::OPERATOR, "=")) {
        advance();
        var->init = parse_pipeline();
        consume(TokenType::SYMBOL, ";");
        return var;
    }

    if(check(TokenType::SYMBOL, ";") || check(TokenType::SYMBOL, ",")) {
        advance();
        return var;
    }
    
    fmt::println(stderr, "\n\nPARSEVARIABLE {} \n\n", get_token().token_value);
  
    //end of crate
    if(check(TokenType::SYMBOL, "}")) {
        consume(TokenType::SYMBOL, "}");
        consume(TokenType::SYMBOL, ";");
        return var;
    }

    if(check(TokenType::SYMBOL, ")")) {
        return var;
    }

    var->init = parse_pipeline();

    consume(TokenType::SYMBOL, ";");
    return var;

}

std::unique_ptr<Node> Parser::parse_incr() {

    Token name_tok = get_token();
    advance();

    if(name_tok.token_type != TokenType::IDENTIFIER) {
        throw std::runtime_error("For loop increment must start with a variable name");
    }
 
    std::unique_ptr<Condition> name_expr = std::make_unique<IdentifierCondition>(name_tok); 
  
    //handles my_struct.name.num
    while (index < length && check(TokenType::SYMBOL, ".")) {
        advance();

        auto field = parse_primary();
        auto dot = std::make_unique<DotNode>();

        dot->left = std::move(name_expr);
        dot->right = std::move(field);
        name_expr = std::move(dot);
    }

    if(check(TokenType::UNARY_OP)) {
        Token op = get_token();

        if(op.token_value == "++" || op.token_value == "--") {
            advance();
            return std::make_unique<UnaryIncrNode>(std::move(name_expr), op.token_value);
        }

    }
    
    if(check(TokenType::OPERATOR)) {
        Token op = get_token();

        if(op.token_value == "=" || op.token_value == "*=" || op.token_value == "*"
            || op.token_value == "/" || op.token_value == "/=" || op.token_value == "+="
            || op.token_value == "+" || op.token_value == "-" || op.token_value == "-=") { 

            auto value = parse_pipeline();

            auto node = std::make_unique<VariableNode>();
            node->name = std::move(name_expr);//identifier (full dot path)
            node->op = op.token_value;//op
            node->init = std::move(value);//right side

            return node;

        }
    }

    throw std::runtime_error("Invalid increment in for loop. Use i++, i-- or assignment");
    
}

std::unique_ptr<ReturnNode> Parser::parse_return() {

    auto rett = std::make_unique<ReturnNode>();

    consume(TokenType::RETURN, "return");

    rett->ret = parse_pipeline();

    consume(TokenType::SYMBOL, ";");
 
    return rett;
}

std::unique_ptr<BodyNode> Parser::parse_body() {
    auto body = std::make_unique<BodyNode>();
    
    consume(TokenType::SYMBOL, "{");
    
    while(!check(TokenType::SYMBOL, "}")) {
        body->statements.push_back(parse_statement());
    }

    consume(TokenType::SYMBOL, "}");

    return body;
}

