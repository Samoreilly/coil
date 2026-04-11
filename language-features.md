# Coil Language

## Overview

Coil is a statically typed, compiled language with object-oriented features, explicit visibility controls, and functional programming patterns.

## Entry Point

`fn main()` is the entry point.

## Core Features

1. Statically typed with type inference
2. Object-oriented with visibility controls
3. Immutable and mutable bindings
4. Explicit, clear naming conventions
5. Pipeline operator (`|>`) for function composition
6. Cascade operator (`..`) for chained operations
7. Defer for guaranteed cleanup
8. Tuples and multiple return values
9. Pattern matching with `match`
10. Import system for C/C++ interop

## Syntax Reference

### Variables

```coil
auto name = "hello";           // type inferred
string mut greeting = "hi";     // mutable
immut int count = 42;          // immutable
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

### Crates (Structs)

```coil
public Crate Point {
    int x;
    int y;
}

Point p = Point::{x: 10, y: 20};
```

### Classes

```coil
public class Greeter {
    private string message;
    int num;

    public Greeter(string msg, int n) {
        message = msg;
        num = n;
    }
}
```

### Control Flow

```coil
if (x > 0) {
    print("positive");
} else if (x < 0) {
    print("negative");
} else {
    print("zero");
}
```

```coil
for (int i = 0; i < 10; i++) {
    print(i);
}

foreach (item in list) {
    print(item);
}

while (condition) {
    // loop body
}
```

### Match Expression

```coil
match (value) {
    is 1: {
        print("one");
        stop;
    }
    is 2: {
        print("two");
        stop;
    }
    default: {
        print("other");
    }
}
```

### Defer

```coil
fn read_file(string path) -> int {
    File mut f = File::open(path);
    defer {
        f.close();
    }

    if (f.is_empty()) {
        return 0;
    }

    return 1;
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

### Tuples and Multiple Returns

```coil
fn get_user() -> (string, int) {
    return ("Alice", 30);
}

(string name, int age) = get_user();
```

## Type System

### Size Types

Signed integers: `i8`, `i16`, `i32`, `i64`, `i128`
Unsigned integers: `u8`, `u16`, `u32`, `u64`, `u128`
Floating point: `f32`, `f64`
Platform-dependent: `int`, `uint`

### Built-in Types

`string`, `int`, `double`, `bool`, `char`, `void`, `pair`

### Arrays

```coil
int arr[5] = {1, 2, 3, 4, 5};
```

## Keywords

`auto`, `immut`, `mut`, `private`, `public`, `fn`, `if`, `else`, `for`, `foreach`, `while`, `match`, `is`, `default`, `stop`, `defer`, `return`, `class`, `Crate`, `pair`
