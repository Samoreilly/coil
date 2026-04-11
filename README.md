# Coil

Coil is a statically typed, compiled language with an imperative core and functional composition features.

## Language Direction

Coil is designed as a hybrid language:

- Imperative by default: explicit control flow, statements, assignments, mutability control.
- Functional where it improves readability: expression composition with pipelines.
- Object and data modeling support through `class` and `crate`.

Two notable influences:

- Elixir-style pipeline: `|>`
- Dart-style cascade: `..`

The goal is to keep execution flow explicit while making transformation-heavy code concise.

## Feature Highlights

### Pipeline (`|>`) and Placeholder (`_`)

```coil
string result = "hello"
    |> String.uppercase(_)
    |> String.trim(_);
```

The piped value is threaded through function calls using `_` as the placeholder.

### Cascade (`..`)

```coil
Greeter mut g = Greeter("hello", 1)
    ..num = 10
    ..message = "hi"
    ..greet();

g = ..num = 10
    ..message = 15;
```

Supported forms:

- Rooted cascade: `receiver ..step ..step`
- Detached cascade assignment: `name = ..step ..step` (receiver inferred from left-hand side)

## Compiler Flow and Architecture

Current compilation flow:

1. File loading
   - Files are loaded by `FileHandler`.
   - File markers are injected so source filename context survives later stages.

2. Lexing
   - The lexer produces tokens with `file_name`, `line`, and `col` metadata.
   - Custom tokens include `PIPELINE`, `CASCADE`, and `PLACEHOLDER`.

3. Parsing
   - A recursive-descent parser builds the AST.
   - Expressions support arithmetic/comparison, pipeline chaining, dot access, and cascades.
   - Statements include declarations, assignments, conditionals, loops, and returns.

4. AST traversal
   - AST nodes are visited via the visitor pattern.
   - A print visitor is currently used for inspection/debugging.

5. Semantic passes (in progress)
   - Symbol registration and type validation are being built in `compiler/middle-end/semantic`.

6. Backend/codegen (planned)
   - Lowering and generation stages are planned after semantic analysis stabilizes.

## Diagnostics and Error Strategy

Coil uses aggregated diagnostics instead of immediate fail-fast throws.

- Each stage reports errors into a shared `Diagnostics` collector.
- Diagnostics include phase, file, line/column, message, and nearby token text.
- Stage boundaries check `has_errors()` and throw a single `CompilationError` with rendered output.

This provides better feedback density per run and keeps error reporting consistent across stages.

## Repository Layout

```text
compiler/
  frontend/
    lexer/
    parser/
  ast/
    visitors/
  middle-end/
    semantic/
  Support/
    Diagnostics/
    ErrorHandling/
  io/
main.cpp
CMakeLists.txt
```

## Build and Run

Requirements:

- C++20 compiler
- CMake
- fmt library

Build:

```bash
cmake -S . -B build
cmake --build build
```

Run:

```bash
./build/coil example.coil
```

## Current Status

Implemented:

- Lexer and parser front-end
- AST and visitor traversal
- Aggregated diagnostics system
- Pipeline and cascade parsing support

In progress:

- Semantic registration and type checking passes
- Constructor/type call resolution in semantic analysis

Planned:

- Stronger semantic validation for pipeline/cascade member operations
- Lowering and backend generation
