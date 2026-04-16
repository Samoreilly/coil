#pragma once

#include "../../frontend/lexer/Token.h"

#include <iostream>
#include <optional>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <utility>

namespace Semantic {

    struct SymbolTable;


    struct SymbolEntry final {
                 
        Token token;
        NodeKind node_kind;
 
        Visibility vis = Visibility::PUBLIC;
        ACCESS access = ACCESS::MUTABLE;

        std::optional<Type> type;
        std::vector<Type> param_types;
        std::map<std::string, int> fields;
        std::shared_ptr<SymbolTable> scope;
        int offset;

        SymbolEntry(
            NodeKind node_kind,
            Visibility vis = Visibility::PUBLIC,
            ACCESS access = ACCESS::MUTABLE,
            std::optional<Type> type = std::nullopt,
            std::vector<Type> param_types = {},
            std::map<std::string, int> fields = {},
            int offset = 0,

            const Token& token = {})
                : token(token),
                node_kind(node_kind),
                vis(vis),
                access(access),
                type(type),
                param_types(std::move(param_types)),
                fields(std::move(fields)),
                offset(offset) {}

        SymbolEntry() = default;

    };

    struct SymbolTable final {

        SymbolTable* parent = nullptr;
        std::string name;

        SymbolTable(std::string n) : name(n) {}

        std::map<std::string, SymbolEntry> entries;

        inline SymbolEntry* lookup(std::string name, bool global) {
            return global ? lookup_global(name) : lookup_local(name);    
        }
        
        inline SymbolEntry* lookup_global(std::string name) {
            if(entries.count(name)) {
                return &entries[name]; 
            }
            if(parent == nullptr) return nullptr;
            return parent->lookup_global(name);
        }

        inline SymbolEntry* lookup_local(std::string name) {
            if(entries.count(name)) {
                return &entries[name];
            }

            return nullptr;
        }

        inline void insert(std::string name, SymbolEntry& entry) {
            entries[name] = entry;
        }

    };



    class Semantic final {


    public:


    };

}
