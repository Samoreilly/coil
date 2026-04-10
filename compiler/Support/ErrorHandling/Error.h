#pragma once

#include "../Diagnostics/Diagnostics.h"

#include <exception>
#include <string>

struct CompilationError final : std::exception {
    std::string message;

    explicit CompilationError(const Diagnostics& diagnostics)
        : message(diagnostics.render()) {}

    explicit CompilationError(std::string m)
        : message(std::move(m)) {}

    const char* what() const noexcept override {
        return message.c_str();
    }
};


