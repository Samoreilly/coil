# Coil Language Longevity Plan

## 1. Syntax Stabilization
The current parser implementation shows ambiguity between `fn` and `duty` keywords. 
- **Action**: Formalize all keywords in a language specification document.
- **Goal**: Ensure a single, unambiguous way to define functions, types, and control flow.

## 2. Memory Management Model
Coil must define its ownership and memory safety model early to avoid the "fragmentation trap" of later adding GC or Borrow Checking.
- **Action**: Decide between Manual (C-style), ARC (Swift-style), or Ownership (Rust-style).
- **Goal**: Zero-cost abstractions that do not compromise performance.

## 3. Error Handling Architecture
Move away from implicit exceptions.
- **Action**: Utilize the existing Tuple/Multiple Return support to implement a Result-based error pattern (e.g., `val, err = duty()`).
- **Goal**: Explicit, predictable error handling that is checked at compile-time.

## 4. First-Class Tooling
A language's adoption is limited by its developer experience.
- **Action**: Implement a Language Server Protocol (LSP) provider as the highest priority tool.
- **Action**: Integrate a standard formatter (`coilfmt`) and a build/dependency manager (`coilpkg`) into the core repository.
- **Goal**: Lower the barrier to entry for new developers.

## 5. Ecosystem Interoperability
Survival depends on the ability to leverage existing libraries.
- **Action**: Implement a "No-Wrapper" C/C++ FFI (Foreign Function Interface) that allows direct import of headers.
- **Goal**: Instant access to established libraries like OpenSSL, SQLite, and system APIs.

## 6. Compiler Robustness
The current handwritten recursive descent parser requires rigorous testing.
- **Action**: Implement a comprehensive test suite for the frontend (lexer/parser) using a tool like LLVM's `lit` or a custom snapshot testing framework.
- **Action**: Establish a CI/CD pipeline that runs these tests on every commit.

## 7. Strategic Documentation
- **Action**: Maintain a living specification that documents the "Why" behind design decisions, not just the "How".
- **Goal**: Provide a roadmap for future contributors to maintain design consistency.
