#pragma once

#include <iostream>
#include <set>
#include <map>

enum class TokenType {
    
    FN, CRATE, VIS, ACCESS, KEYWORD, DO, END, CLASS,

    TYPE_KEYWORD, 
    STRING_LITERAL, BOOL_LITERAL, DOUBLE_LITERAL, INTEGER_LITERAL, CHAR_LITERAL, 
    
    IDENTIFIER, RETURN, CONSTRUCTOR,

    FOR, FOREACH, WHILE, IF, ELSE, ELSEIF, MATCH,

    OPERATOR, SYMBOL, UNARY_OP, LOGICAL_OP, BITWISE_OP,

    //CUSTOM FEATURES
//    ..       |>
    CASCADE, PIPELINE,

    ARROW,

    END_OF_FILE

};

enum class Visibility : int {PUBLIC, PRIVATE};
enum class ACCESS : int {IMMUTABLE, MUTABLE};
enum class TYPE : int {STRING, INT, DOUBLE, CHAR, BOOL, CRATE, VOID, PAIR};

static const std::map<TokenType, std::string> tokenTypeToString = {

    {TokenType::FN, "FN"},
    {TokenType::CRATE, "CRATE"},
    {TokenType::VIS, "VIS"},
    {TokenType::ACCESS, "ACCESS"},
    {TokenType::KEYWORD, "KEYWORD"},
    {TokenType::DO, "DO"},
    {TokenType::END, "END"},
    {TokenType::CLASS, "CLASS"},

    {TokenType::TYPE_KEYWORD,   "TYPE_KEYWORD"},
    {TokenType::STRING_LITERAL, "STRING_LITERAL"},
    {TokenType::BOOL_LITERAL, "BOOL_LITERAL"},
    {TokenType::DOUBLE_LITERAL, "DOUBLE_LITERAL"},
    {TokenType::INTEGER_LITERAL, "INTEGER_LITERAL"},
    {TokenType::CHAR_LITERAL, "CHAR_LITERAL"},

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

static const inline std::map<std::string_view, Visibility> to_vis_enum = {
    {"private", Visibility::PRIVATE},
    {"public", Visibility::PUBLIC}
};

static const inline std::set<std::string> RESERVED = {
    "fn", "Crate", "Class", "for", "if", "while", "mut", "immut", "do", "end",
};

static const inline std::set<std::string> VISIBILITY = {
    "public", "private"
};

static const inline std::map<std::string, TYPE> TYPES = {
    {"string",      TYPE::STRING},
    {"int",         TYPE::INT},
    {"double",      TYPE::DOUBLE},
    {"bool",        TYPE::BOOL},
    {"char",        TYPE::CHAR},
    {"crate",       TYPE::CRATE},
    {"pair",        TYPE::PAIR},
    {"void",        TYPE::VOID},
};

struct Token {
    TokenType token_type;
    std::string token_value;
    int line;
    int col;
};
