#pragma once

#include "../../ast/Node.h"
#include "Semantic.h"

static inline bool same_type(const Type& lhs, const Type& rhs) {
    return lhs.type == rhs.type &&
        lhs.bit_width == rhs.bit_width &&
        lhs.is_signed == rhs.is_signed &&
        lhs.name == rhs.name;
}

static inline bool is_numeric_type(const Type& type) {
    return type.type == TypeCategory::INT || type.type == TypeCategory::FLOAT;
}

static inline bool type_allows_ordering(const Type& type) {
    return is_numeric_type(type) || type.type == TypeCategory::CHAR;
}

static inline bool is_arithmetic_operator(const std::string& op) {
    return op == "+" || op == "-" || op == "*" || op == "/";
}

static inline bool is_compound_assignment_operator(const std::string& op) {
    return op == "+=" || op == "-=" || op == "*=" || op == "/=";
}

static inline bool is_unary_update_operator(const std::string& op) {
    return op == "++" || op == "--";
}

static inline bool is_assignment_operator(const std::string& op) {
    return op == "=" || is_compound_assignment_operator(op);
}

static inline bool is_comparison_operator(const std::string& op) {
    return op == "<" || op == ">" || op == "<=" || op == ">=" || op == "==" || op == "!=";
}

static inline bool is_ordering_comparison_operator(const std::string& op) {
    return op == "<" || op == ">" || op == "<=" || op == ">=";
}

static inline Type parameter_type(const std::unique_ptr<Parameter>& param) {
    if (!param) {
        return {TypeCategory::VOID, 0, false, "void"};
    }

    if (std::holds_alternative<Type>(param->type)) {
        return std::get<Type>(param->type);
    }

    return {TypeCategory::CLASS, 0, false, std::get<std::string>(param->type)};
}

static inline std::vector<Type> parameter_types(const std::vector<std::unique_ptr<Parameter>>& params) {
    std::vector<Type> types;
    types.reserve(params.size());

    for (const auto& param : params) {
        types.push_back(parameter_type(param));
    }

    return types;
}

static inline Semantic::SymbolTable* bind_body_scope(BodyNode& body, Semantic::SymbolTable* parent) {
    if (!body.scope) {
        body.scope = std::make_shared<Semantic::SymbolTable>(parent ? parent->name + ".body" : "body");
        body.scope->parent = parent;
    }

    return body.scope.get();
}

static inline bool same_signature(const std::vector<Type>& lhs, const std::vector<Type>& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t i = 0; i < lhs.size(); ++i) {
        if (!same_type(lhs[i], rhs[i])) {
            return false;
        }
    }

    return true;
}

static inline bool can_implicitly_convert(const Type& from, const Type& to) {
    if (same_type(from, to)) {
        return true;
    }

    if (from.type == TypeCategory::INT && to.type == TypeCategory::INT) {
        return from.is_signed == to.is_signed && from.bit_width <= to.bit_width;
    }

    if (from.type == TypeCategory::INT && to.type == TypeCategory::FLOAT) {
        return true;
    }

    if (from.type == TypeCategory::FLOAT && to.type == TypeCategory::FLOAT) {
        return from.bit_width <= to.bit_width;
    }

    return false;
}

static inline std::optional<Type> common_numeric_type(const Type& lhs, const Type& rhs) {
    if (!is_numeric_type(lhs) || !is_numeric_type(rhs)) {
        return std::nullopt;
    }

    if (same_type(lhs, rhs)) {
        return lhs;
    }

    if (lhs.type == TypeCategory::FLOAT || rhs.type == TypeCategory::FLOAT) {
        if (lhs.type == TypeCategory::FLOAT && rhs.type == TypeCategory::FLOAT) {
            if (lhs.bit_width >= rhs.bit_width) return lhs;
            return rhs;
        }

        if (lhs.type == TypeCategory::FLOAT && rhs.type == TypeCategory::INT) {
            return lhs;
        }

        if (lhs.type == TypeCategory::INT && rhs.type == TypeCategory::FLOAT) {
            return rhs;
        }

        return std::nullopt;
    }

    if (lhs.type == TypeCategory::INT && rhs.type == TypeCategory::INT) {
        if (lhs.is_signed != rhs.is_signed) {
            return std::nullopt;
        }

        if (lhs.bit_width >= rhs.bit_width) return lhs;
        return rhs;
    }

    return std::nullopt;
}
