
#include "../../ast/Node.h"
#include "Parser.h"
#include <fmt/core.h>

std::unique_ptr<GlobalNode> Parser::construct_ast() {
    auto root_node = std::make_unique<GlobalNode>();

    while(index < length && !check(TokenType::END_OF_FILE)) {
        int start_index = index;
        auto statement = parse_statement();

        if (statement) {
            root_node->globals.push_back(std::move(statement));
        }

        if (index == start_index && !check(TokenType::END_OF_FILE)) {
            advance();
        }
    }

    return root_node;

}


std::unique_ptr<Node> Parser::parse_statement() {
    Token curr = get_token();

    switch(curr.token_type) {

        case TokenType::FN: 
            return parse_fn();
        case TokenType::CRATE: 
            return parse_crate();
        

        //variable or function
        case TokenType::VIS: {
            
            // save the visibility token
            std::string first_vis = curr.token_value;
            advance();

            if(check(TokenType::FN, "fn")) {    
                return parse_fn(first_vis);

            }else if(check(TokenType::CLASS, "class")){
                return parse_class(first_vis);

            }else if(check(TokenType::ACCESS)){
                return parse_variable(first_vis, get_token().token_value);
            }else if(check(TokenType::CRATE)) {
                return parse_crate(first_vis);
            }else if(check(TokenType::IDENTIFIER) && peek_next(1).token_type == TokenType::SYMBOL && peek_next(1).token_value == "(") {
                // constructor with visibility
                return std::unique_ptr<Node>(parse_fn_call(first_vis).release());
            }else if(check(TokenType::TYPE_KEYWORD)) {
                return parse_variable(first_vis);
            }else if(check(TokenType::KEYWORD, "auto")) {
                return parse_variable(first_vis);
            }else {
                Token bad = get_token();
                report_error(
                    "Invalid token '" + bad.token_value +
                    "' after visibility '" + first_vis + "'",
                    bad
                );
                synchronize_statement();
                return nullptr;
            }
            
            break;
        }

        case TokenType::ACCESS: {
            return parse_variable("", curr.token_value);
        }

        case TokenType::TYPE_KEYWORD: return parse_variable();

        case TokenType::KEYWORD:
            if (check(TokenType::KEYWORD, "auto")) {
                return parse_variable();
            }
            break;

        case TokenType::IDENTIFIER: {

            
            if ((peek_next(1).token_type == TokenType::ACCESS) ||
                (peek_next(1).token_type == TokenType::TYPE_KEYWORD) ||
                (peek_next(1).token_type == TokenType::VIS)) {
                return parse_variable();
            }

            //constructor look ahead
            if(peek_next(1).token_type == TokenType::SYMBOL && peek_next(1).token_value == "(") {
                
                int tmp_idx = index + 1;
                int paren_depth = 0;
                
                while(tmp_idx < length) {
                    if (tokens[tmp_idx].token_value == "(") {
                        paren_depth++;
                    } else if (tokens[tmp_idx].token_value == ")") {
                        paren_depth--;
                        if (paren_depth == 0) {
                            tmp_idx++;
                            break;
                        }
                    }
                    tmp_idx++;
                }
                
                if(tmp_idx < length) {
                    if(tokens[tmp_idx].token_value == "{") {
                        // constructor def
                        return std::unique_ptr<Node>(parse_fn_call().release());
                    } else {
                        // function call, pipeline, or cascade
                        auto expr = parse_pipeline();
                            
                        //called after constructor
                        while (check(TokenType::CASCADE)) {
                            expr = parse_cascade(std::move(expr));
                        }

                        if (check(TokenType::SYMBOL, ";")) {
                            consume(TokenType::SYMBOL);
                        }
                        return std::unique_ptr<Node>(expr.release());
                    }
                }
            }

            if (peek_next(1).token_type == TokenType::CASCADE) {
                auto expr = parse_pipeline();
                if (check(TokenType::SYMBOL, ";")) {
                    consume(TokenType::SYMBOL, ";");
                }
                return std::unique_ptr<Node>(expr.release());
            }
   
            if (peek_next(1).token_value == ".") {
                auto expr = parse_pipeline();

                if (check(TokenType::OPERATOR) && assign_ops.count(get_token().token_value)) {
                    std::string op = get_token().token_value;
                    advance();
                    
                    auto val = parse_pipeline();
                    auto node = std::make_unique<VariableNode>();

                    node->name = std::move(expr);
                    node->op = op;
                    node->init = std::move(val);

                    if (check(TokenType::SYMBOL, ";")) {
                        consume(TokenType::SYMBOL);
                    }

                    return node;
                }

                if (check(TokenType::SYMBOL, ";")) {
                    consume(TokenType::SYMBOL);
                }

                return std::unique_ptr<Node>(expr.release());
            }

            if (assign_ops.count(peek_next(1).token_value)) {
                Token name_tok = get_token();
                advance(); // consume identifier
                std::string op = get_token().token_value;
                advance(); // consume operator
                std::unique_ptr<Condition> val;

                //no prior constructor just g ==  ..num = 15 ..msg = "test"
                if (check(TokenType::CASCADE)) {
                    auto base = std::make_unique<IdentifierCondition>(name_tok);
                    val = parse_cascade(std::move(base));
                } else {
                    val = parse_pipeline();
                }

                auto node = std::make_unique<VariableNode>();
                node->name = std::make_unique<IdentifierCondition>(name_tok);
                node->op = op;
                node->init = std::move(val);
                if (check(TokenType::SYMBOL, ";")) consume(TokenType::SYMBOL);
                return node;
            }

            return parse_variable();
        }
        case TokenType::RETURN: return parse_return();
        case TokenType::CONSTRUCTOR:
            report_error("Standalone constructor token is not valid here", curr);
            advance();
            return nullptr;

        case TokenType::FOR: return parse_for();
        case TokenType::FOREACH:
            report_error("'foreach' is not implemented yet", curr);
            synchronize_statement();
            return nullptr;
        case TokenType::WHILE: return parse_while();
        case TokenType::IF: return parse_if();
        case TokenType::ELSE:
            report_error("Unexpected 'else' without matching 'if'", curr);
            synchronize_statement();
            return nullptr;
        case TokenType::ELSEIF:
            report_error("Unexpected 'elseif' without matching 'if'", curr);
            synchronize_statement();
            return nullptr;
        case TokenType::MATCH:
            report_error("'match' is not implemented yet", curr);
            synchronize_statement();
            return nullptr;

        case TokenType::SYMBOL: {
            if (curr.token_value == ";") {
                advance();
                return nullptr;
            }else if(curr.token_value == "{" && peek_next(1).token_value == "}") {
                advance();
                advance();
                return nullptr;
            }
            report_error("Unexpected symbol in statement: " + curr.token_value, curr);
            advance();
            return nullptr;
        }

        case TokenType::PIPELINE: {
            return std::unique_ptr<Node>(parse_pipeline().release());
        }

        case TokenType::END_OF_FILE:
            return nullptr;

        default:
            report_error("Unexpected token in statement: " + curr.token_value, curr);
            advance();
            return nullptr;
    };

    return nullptr;
}

Visibility Parser::handle_visibility(const std::string_view vis) {
    if (vis.empty()) {
        return Visibility::PUBLIC;
    }

    auto it = to_vis_enum.find(vis);
    if (it != to_vis_enum.end()) {
        return it->second;
    }

    Token curr = get_token();
    report_error("Unknown visibility: '" + std::string(vis) + "'", curr);
    return Visibility::PUBLIC;
}

std::unique_ptr<ClassNode> Parser::parse_class(const std::string_view vis) {
    auto cl = std::make_unique<ClassNode>();

    if(!vis.empty()) {
        cl->vis = handle_visibility(vis);
    }

    consume(TokenType::CLASS, "class");

    cl->name = get_token().token_value;
    advance();


    cl->body = parse_body();
    
    if (check(TokenType::SYMBOL, ";")) {
        consume(TokenType::SYMBOL, ";");
    }

    return cl;
}

std::unique_ptr<FnNode> Parser::parse_fn(const std::string_view vis) {
    
    auto fn = std::make_unique<FnNode>();

    fn->vis = handle_visibility(vis);
    
    consume(TokenType::FN, "fn");
    fn->name = get_token().token_value;
    advance();

    consume(TokenType::SYMBOL, "(");

    //parameters
    while(!check(TokenType::SYMBOL, ")") && !check(TokenType::END_OF_FILE)) {
        
        if(check(TokenType::SYMBOL, ",")) {
            consume(TokenType::SYMBOL); 
        }

        auto param = std::make_unique<Parameter>("test");
        
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

    if (check(TokenType::END_OF_FILE)) {
        report_error("Expected ')' to close function parameter list", get_token());
        return fn;
    }

    consume(TokenType::SYMBOL, ")");
    
    if(check(TokenType::ARROW, "->")) {
        consume(TokenType::ARROW, "->");
        //return type
        Token ret = get_token();
        advance();
        
        if(TYPES.find(ret.token_value) == TYPES.end()) {
            report_error("Unknown return type: " + ret.token_value, ret);
            return fn;
        }

        auto it = TYPES.find(ret.token_value);
        if (it != TYPES.end()) {
            fn->return_type = it->second;
        }
    }

    fn->body = parse_body();

    return fn;

}


std::unique_ptr<CrateNode> Parser::parse_crate(const std::string_view vis) {
    auto crate = std::make_unique<CrateNode>();

    if(!vis.empty()) {
        crate->vis = handle_visibility(vis);
    }
    
    consume(TokenType::CRATE, "crate");

    crate->name = get_token().token_value;
    advance();

    consume(TokenType::SYMBOL, "{");

    while(!check(TokenType::SYMBOL, "}") && !check(TokenType::END_OF_FILE)) {
        crate->crate_vars.push_back(parse_variable());
    }

    if (check(TokenType::END_OF_FILE)) {
        report_error("Expected '}' to close crate body", get_token());
        return crate;
    }

    consume(TokenType::SYMBOL, "}");
    if (check(TokenType::SYMBOL, ";")) {
        consume(TokenType::SYMBOL, ";");
    }

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

    auto var = std::make_unique<VariableNode>();
    bool has_declared_type = false;
        
    //public or private
    if(!vis.empty()) {
        auto it = to_vis_enum.find(vis);
        if (it != to_vis_enum.end()) {
            var->vis = it->second;
        } else {
            report_error("Unknown visibility for variable: '" + std::string(vis) + "'", get_token());
        }
    }

    //access immutable or mutable
    if(!access.empty()) {
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
            has_declared_type = true;
        } else {
            report_error("Unknown variable type: '" + get_token().token_value + "'", get_token());
        }

        consume(TokenType::TYPE_KEYWORD);

    } else if (check(TokenType::KEYWORD, "auto")) {
        var->inferred_type = true;
        advance();

    } else if(check(TokenType::VIS)) {
        auto vis = get_token().token_value;
        advance();

        if(check(TokenType::TYPE_KEYWORD)) {
            auto it = TYPES.find(get_token().token_value);
            if (it != TYPES.end()) {
                var->type = it->second;
                has_declared_type = true;
                var->vis = (vis == "public") ? Visibility::PUBLIC : Visibility::PRIVATE;
            }
            consume(TokenType::TYPE_KEYWORD);
        }

    } else if (check(TokenType::IDENTIFIER)) {
        Token object_type = get_token();
        if (peek_next(1).token_type == TokenType::ACCESS ||
            peek_next(1).token_type == TokenType::IDENTIFIER) {
            var->type = Type{TypeCategory::CLASS, 0, false, object_type.token_value};
            has_declared_type = true;
            advance();

            if (check(TokenType::ACCESS)) {
                var->access = (get_token().token_value == "immut")
                    ? ACCESS::IMMUTABLE
                    : ACCESS::MUTABLE;
                consume(TokenType::ACCESS);
            }
        }
    }


    Token name_tok = get_token();
    if (name_tok.token_type != TokenType::IDENTIFIER) {
        report_error("Expected variable name", name_tok);
        if (!check(TokenType::END_OF_FILE)) {
            advance();
        }
        return var;
    }

    advance();
    std::unique_ptr<Condition> name_expr = std::make_unique<IdentifierCondition>(name_tok); 


    var->name = std::move(name_expr);

    //prior declaration, now initializing e.g. x = 10;
    if(check(TokenType::OPERATOR, "=")) {
        advance();
        var->init = parse_pipeline();
        consume(TokenType::SYMBOL, ";");
        return var;
    }

    //checks for no type or declaration
    if(check(TokenType::SYMBOL, ";") || check(TokenType::SYMBOL, ",")) {
        if (!has_declared_type && !var->inferred_type) {
            report_error("Variable declaration requires a type or initializer", name_tok);
        }
        advance();
        return var;
    }
    
    //end of crate
    if(check(TokenType::SYMBOL, "}")) {
        report_error("Unexpected '}' while parsing variable declaration", get_token());
        advance();
        return var;
    }

    if(check(TokenType::SYMBOL, ")")) {
        if (!has_declared_type && !var->inferred_type) {
            report_error("Variable declaration requires a type or initializer", name_tok);
        }
        return var;
    }

    var->init = parse_pipeline();

    consume(TokenType::SYMBOL, ";");
    return var;

}

std::unique_ptr<Node> Parser::parse_incr() {
    // parse the full name expression (identifier or dot chain)
    std::unique_ptr<Condition> name_expr = parse_pipeline();

    if (check(TokenType::UNARY_OP)) {
        Token op = get_token();
        if (op.token_value == "++" || op.token_value == "--") {
            advance();
            return std::make_unique<UnaryIncrNode>(std::move(name_expr), op.token_value);
        }
    }

    if (check(TokenType::OPERATOR) && assign_ops.count(get_token().token_value)) {
        std::string op = get_token().token_value;
        advance();
        auto value = parse_pipeline();
        auto node = std::make_unique<VariableNode>();
        node->name = std::move(name_expr);
        node->op = op;
        node->init = std::move(value);
        return node;
    }

    report_error("Invalid increment in for loop. Use i++, i-- or assignment", get_token());
    return nullptr;
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
    
    while(!check(TokenType::SYMBOL, "}") && !check(TokenType::END_OF_FILE)) {
        auto statement = parse_statement();
        if (statement) {
            body->statements.push_back(std::move(statement));
        }
    }

    if (check(TokenType::END_OF_FILE)) {
        report_error("Expected '}' to close block", get_token());
        return body;
    }

    consume(TokenType::SYMBOL, "}");

    return body;
}
