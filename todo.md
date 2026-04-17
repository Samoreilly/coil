1. Refactor type system to support extensible integral types | DONE
2. Replace property-based Type class instead of enum         | DONE
3. Update lexer to use KEYWORDS map for keyword recognition  | DONE
4. Extract operator and symbol handling into lookup tables   | DONE
5. Improve parser modularity for easier language extensions  |
6. Add unit tests for lexer and parser components            | DONE
7. Verify all documented language features are implemented   | Match soon
8. Ensure build system supports debug and release profiles
9. Add source location tracking to all error messages        | DONE
10. Reduce code duplication in lexer implementation          | DONE
11. Remove debug print spam from lexer and parser            | DONE
12. Remove duplicate keyword checks after KEYWORDS map       | DONE
13. Fix keyword token column to use start column instead of end column
15. Replace constructor detection heuristics with explicit constructor parsing in class bodies | DONE
16. Remove double visibility fallback and throw clear syntax errors | DONE 
17. Fix parse_variable so it never consumes class or crate closing braces | DONE
18. Make parse_primary throw parse errors instead of returning StringCondition by default | DONE
19. Add regression tests for constructor params, function calls, and visibility parsing | DONE
20. Sync language-features.md examples with real parser grammar
21. Add detailed diagnostics/error handling | DONe



# Today
1. Implement cascade parsing, functionality
2. Work on semantic analysis 1st pass - type checking
