#pragma once

#include <iostream>
#include <set>
#include <map>

enum class TokenType {
    
    DUTY, CRATE, VIS, ACCESS, KEYWORD, DO, END,

    TYPE, STRING, BOOL, DOUBLE, INTEGER, CHAR, VOID, PAIR,
    
    IDENTIFIER, RETURN, CONSTRUCTOR,

    FOR, FOREACH, WHILE, IF, ELSE, ELSEIF, MATCH,

    OPERATOR, SYMBOL, UNARY_OP, LOGICAL_OP, BITWISE_OP,

    //CUSTOM FEATURES
//    ..       |>
    CASCADE, PIPELINE,

    END_OF_FILE

};

static const std::map<TokenType, std::string> tokenTypeToString = {

    {TokenType::DUTY, "DUTY"},
    {TokenType::CRATE, "CRATE"},
    {TokenType::VIS, "VIS"},
    {TokenType::ACCESS, "ACCESS"},
    {TokenType::KEYWORD, "KEYWORD"},
    {TokenType::DO, "DO"},
    {TokenType::END, "END"},

    {TokenType::TYPE,   "TYPE"},
    {TokenType::STRING, "STRING"},
    {TokenType::BOOL, "BOOL"},
    {TokenType::DOUBLE, "DOUBLE"},
    {TokenType::INTEGER, "INTEGER"},
    {TokenType::CHAR, "CHAR"},
    {TokenType::VOID, "VOID"},
    {TokenType::PAIR, "PAIR"},

    {TokenType::IDENTIFIER, "IDENTIFIER"},
    {TokenType::RETURN, "RETURN"},
    {TokenType::CONSTRUCTOR, "CONSTRUCTOR"},

    {TokenType::FOR, "FOR"},
    {TokenType::FOREACH, "FOREACH"},
    {TokenType::WHILE, "WHILE"},
    {TokenType::IF, "IF"},
    {TokenType::ELSE, "ELSE"},
    {TokenType::ELSEIF, "ELSEIF"},
    {TokenType::MATCH, "MATCH"},

    {TokenType::OPERATOR, "OPERATOR"},
    {TokenType::SYMBOL, "SYMBOL"},
    {TokenType::UNARY_OP, "UNARY_OP"},
    {TokenType::LOGICAL_OP, "LOGICAL_OP"},
    {TokenType::BITWISE_OP, "BITWISE_OP"},

    {TokenType::CASCADE, "CASCADE"},
    {TokenType::PIPELINE, "PIPELINE"},

    {TokenType::END_OF_FILE, "END_OF_FILE"}
};


static const inline std::set<std::string> RESERVED = {
    "duty", "for", "if", "while", "mut", "immut", "do", "end",
};

static const inline std::set<std::string> VISIBILITY = {
    "public", "private"
};

static const inline std::map<std::string, TokenType> TYPES = {
    {"string",      TokenType::STRING},
    {"int",         TokenType::INTEGER},
    {"double",      TokenType::DOUBLE},
    {"bool",        TokenType::BOOL},
    {"char",        TokenType::CHAR},
    {"crate",       TokenType::CRATE},
    {"pair",        TokenType::PAIR},
    {"void",        TokenType::VOID},
};

struct Token {
 
    TokenType token_type;
    std::string token_value;
    int line;
    int col;
};
