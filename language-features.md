# Coil Language

## Overview

Coil is a statically typed, compiled language with object-oriented features, explicit visibility controls, and functional programming patterns.

## Entry Point

`fn main()` is the entry point.

## Core Features

1. Statically typed with type inference
2. Object-oriented with visibility controls
3. Immutable and mutable bindings
4. Pipeline operator (`|>`) for function composition
5. Cascade operator (`..`) for chained operations
6. Import system for C/C++ interop

## Syntax Reference

### Variables

```coil
auto name = "hello";           // type inferred
mut string greeting = "hi";    // mutable
immut int count = 42;           // immutable
```

### Functions

```coil
private fn add(int a, int b) -> int {
    return a + b;
}

public fn greet(string name) {
    print(name);
}
```

### Crates

```coil
private crate Point {
    int x;
    int y;
}

Point p = Point(10, 20);
```

### Classes

```coil
public class Greeter {
    private string message;
    int num;

    private Greeter(string msg, int n) {
        message = msg;
        num = n;
    }
}
```

### Control Flow

```coil
if (x > 0) {
    print("positive");
} elseif (x < 0) {
    print("negative");
} else {
    print("zero");
}
```

```coil
for (int i = 0; i < 10; i++) {
    print(i);
}

while (condition) {
    // loop body
}
```

### Pipeline and Placeholders

```coil
string result = "hello" |> String.uppercase(_) |> String.trim(_);
```

### Cascade

```coil
Greeter mut g = Greeter("hello", 1)
    ..num = 10
    ..message = "hi"
    ..greet();


g = ..num = 10
    ..message = 15;

```

## Type System

### Size Types

Signed integers: `i8`, `i16`, `i32`, `i64`, `i128`
Unsigned integers: `u8`, `u16`, `u32`, `u64`, `u128`
Floating point: `f32`, `f64`
Platform-dependent: `int`, `uint`

### Built-in Types

`string`, `int`, `bool`, `char`, `void`, `pair`

### Arrays

```coil
int arr[5] = {1, 2, 3, 4, 5};
```

## Keywords

`auto`, `immut`, `mut`, `private`, `public`, `fn`, `if`, `elseif`, `else`, `for`, `foreach`, `while`, `match`, `defer`, `return`, `class`, `crate`, `pair`
