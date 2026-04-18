# Next
1. Add a real `defer` implementation if it remains part of the language contract
2. Decide whether tuple/multiple-return syntax should be implemented or removed from the language spec
3. Fix the `Parser` member initialization order warning in `Parser.h`
4. Split semantic type resolution into narrower helpers if new conversions are added later
5. Add a proper release artifact layout instead of relying on ad hoc build outputs
6. Improve match diagnostics to highlight the exact failing case/pattern in parser and semantic errors
7. Add coverage for nested `match` expressions and arm-local variable declarations in semantic tests
8. Decide whether `example.coil` should stay feature-heavy or be trimmed to a smaller reference sample
