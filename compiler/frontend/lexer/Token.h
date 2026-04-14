#pragma once

#include <iostream>
#include <unordered_set>
#include <set>
#include <map>

enum class TokenType {

    FN, CRATE, VIS, ACCESS, KEYWORD, CLASS,

    TYPE_KEYWORD, 
    STRING_LITERAL, BOOL_LITERAL, DOUBLE_LITERAL, INTEGER_LITERAL, CHAR_LITERAL, 
    
    IDENTIFIER, RETURN, CONSTRUCTOR,

    FOR, FOREACH, WHILE, IF, ELSE, ELSEIF, MATCH,

    OPERATOR, SYMBOL, UNARY_OP, LOGICAL_OP, BITWISE_OP,

    //CUSTOM FEATURES
//    ..       |>
    CASCADE, 
    PIPELINE, PLACEHOLDER, //arg placement in function call int x = 4 |> multiply(2, _);

    ARROW,

    FILE_MARKER,
    END_OF_FILE

};

enum class Visibility : int { PUBLIC, PRIVATE };
enum class ACCESS : int { IMMUTABLE, MUTABLE };
enum class TypeCategory : int { STRING, INT, FLOAT, CHAR, BOOL, CLASS, CRATE, VOID, PAIR };
enum class NodeKind : int {
    GLOBAL_NODE,
    VARIABLE_NODE,
    BINARY_EXPRESSION,
    FN_NODE,
    FN_CALL_NODE,
    CONSTRUCTOR_NODE,
    BODY_NODE,
    CLASS_NODE,
    CRATE_NODE,
    DOT_NODE,
    UNARY_INCR_NODE,
    FOR_NODE,
    WHILE_NODE,
    IF_NODE,
    ELSE_IF_NODE,
    ELSE_NODE,
    MATCH_NODE,
    PIPELINE_NODE,
    CASCADE_NODE,
    RETURN_NODE,
    IDENTIFIER_CONDITION,
    INTEGER_CONDITION,
    DOUBLE_CONDITION,
    BOOL_CONDITION,
    STRING_CONDITION,
    CHAR_CONDITION
};

struct Type {
    TypeCategory type;
    int bit_width;
    bool is_signed;
    std::string name;//i32, u32, f64

    Type(TypeCategory tc, int w, bool is_s, std::string n) : type(tc), bit_width(w), is_signed(is_s), name(n) {}
};

static const std::map<TokenType, std::string> tokenTypeToString = {

    {TokenType::FN, "FN"},
    {TokenType::CLASS, "CLASS"},
    {TokenType::CRATE, "CRATE"},
    {TokenType::VIS, "VIS"},
    {TokenType::ACCESS, "ACCESS"},
    {TokenType::KEYWORD, "KEYWORD"},
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
    {TokenType::ARROW, "ARROW"},

    {TokenType::PLACEHOLDER, "PLACEHOLDER"},
    {TokenType::FILE_MARKER, "FILE_MARKER"},
    {TokenType::END_OF_FILE, "END_OF_FILE"}
};

static const inline std::map<std::string_view, Visibility> to_vis_enum = {
    {"private", Visibility::PRIVATE},
    {"public", Visibility::PUBLIC}
};

static const inline std::set<std::string> ACCESS_MAP {
    "immut", "mut"
};

static const inline std::map<std::string, TokenType> KEYWORDS = {
    {"fn",       TokenType::FN},
    {"crate",    TokenType::CRATE},
    {"class",    TokenType::CLASS},
    {"return",   TokenType::RETURN},
    {"for",      TokenType::FOR},
    {"foreach",  TokenType::FOREACH},
    {"while",    TokenType::WHILE},
    {"if",       TokenType::IF},
    {"elseif",   TokenType::ELSEIF},
    {"else",     TokenType::ELSE},
    {"match",    TokenType::MATCH},
};

static const inline std::set<std::string> RESERVED = {
    "fn", "crate", "class",
    "for", "foreach", "while",
    "if", "elseif", "else",
    "match", "return"
};

static const inline std::set<std::string> VISIBILITY = {
    "public", "private"
};

static const std::unordered_set<std::string> assign_ops = {
    "=", "+=", "-=", "*=", "/="
};
 
static const inline std::map<std::string, Type> TYPES = {
    // integral types - signed
    {"i8",          {TypeCategory::INT, 8, true, "i8"}},
    {"i16",         {TypeCategory::INT, 16, true, "i16"}},
    {"i32",         {TypeCategory::INT, 32, true, "i32"}},
    {"i64",         {TypeCategory::INT, 64, true, "i64"}},
    {"i128",        {TypeCategory::INT, 128, true, "i128"}},
    
    // integral types - unsigned
    {"u8",          {TypeCategory::INT, 8, false, "u8"}},
    {"u16",         {TypeCategory::INT, 16, false, "u16"}},
    {"u32",         {TypeCategory::INT, 32, false, "u32"}},
    {"u64",         {TypeCategory::INT, 64, false, "u64"}},
    {"u128",        {TypeCategory::INT, 128, false, "u128"}},
    
    {"int",         {TypeCategory::INT, 32, true, "int"}},    // Assuming 32-bit platform
    {"uint",        {TypeCategory::INT, 32, false, "uint"}},  // Assuming 32-bit platform
    
    // floating point types
    {"f32",         {TypeCategory::FLOAT, 32, false, "f32"}},
    {"f64",         {TypeCategory::FLOAT, 64, false, "f64"}},
    
    // other basic types
    {"string",      {TypeCategory::STRING, 0, false, "string"}},
    {"bool",        {TypeCategory::BOOL, 1, false, "bool"}},
    {"char",        {TypeCategory::CHAR, 8, false, "char"}},
    {"pair",        {TypeCategory::PAIR, 0, false, "pair"}},
    {"void",        {TypeCategory::VOID, 0, false, "void"}}
};

struct Token {
    TokenType token_type;
    std::string token_value;
    std::string file_name;
    int line;
    int col;
    
};


