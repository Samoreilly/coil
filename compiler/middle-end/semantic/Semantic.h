
#include "../../frontend/lexer/Token.h"

#include <iostream>
#include <optional>
#include <memory>
#include <map>

namespace Semantic {


    struct SymbolEntry final {
        std::string name; 
        std::optional<Type> type;
        
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
            return parent->lookup_global(name);
        }

        inline SymbolEntry* lookup_local(std::string name) {
            if(entries.count(name)) {
                return &entries[name];
            }

            return nullptr;
        }


    };



    class Semantic final {


    public:


    };

}
