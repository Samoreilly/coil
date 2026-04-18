#pragma once

#include "../../frontend/lexer/Token.h"

#include <cstddef>
#include <stdexcept>

namespace backend::codegen {

enum class ScalarKind {
    Void,
    Integer,
    Floating,
    Pointer,
};

struct ValueLayout final {
    ScalarKind kind = ScalarKind::Integer;
    std::size_t bit_width = 64;
    bool is_signed = true;
    std::size_t machine_bytes = 8;
};

inline ValueLayout layout_for_type(const Type& type) {
    switch (type.type) {
        case TypeCategory::VOID:
            return {ScalarKind::Void, 0, false, 0};
        case TypeCategory::BOOL:
        case TypeCategory::CHAR:
        case TypeCategory::INT:
            if (type.bit_width <= 64) {
                return {ScalarKind::Integer, static_cast<std::size_t>(type.bit_width), type.is_signed, 8};
            }
            throw std::runtime_error("unsupported integer width: " + type.name);
        case TypeCategory::FLOAT:
            if (type.bit_width <= 32) {
                return {ScalarKind::Floating, 32, false, 4};
            }

            if (type.bit_width <= 64) {
                return {ScalarKind::Floating, 64, false, 8};
            }

            throw std::runtime_error("unsupported float width: " + type.name);
        case TypeCategory::STRING:
        case TypeCategory::CLASS:
        case TypeCategory::CRATE:
        case TypeCategory::PAIR:
            return {ScalarKind::Pointer, 64, false, 8};
    }

    throw std::runtime_error("unsupported type: " + type.name);
}

inline bool is_integer_like(const ValueLayout& layout) {
    return layout.kind == ScalarKind::Integer || layout.kind == ScalarKind::Pointer;
}

inline bool is_floating_like(const ValueLayout& layout) {
    return layout.kind == ScalarKind::Floating;
}

inline std::size_t slot_bytes(const ValueLayout& layout) {
    return layout.kind == ScalarKind::Void ? 0 : 8;
}

inline std::size_t slot_bytes(const Type& type) {
    return slot_bytes(layout_for_type(type));
}

} // namespace backend::codegen
