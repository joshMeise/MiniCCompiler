# MiniCCompiler

## MiniC Syntax Guide

MiniC is a subset of the C programming language. It followes the same syntactic guidelines as C with a few minor modifications:
- Each file contains only one function
- Each file contains the following prelude:
    ```C
    extern void print(int);
    extern int read();
    ```
- Functions may only take 0 or 1 argument
- `while` loops are permitted; `for` loops are not permitted
- Only integer variables are permitted
- Function return types may only be of type `int`
- Arithmetic and relational expressions may only take 2 arguments
- Variable declarations and assignments cannot take place on the same line
- Only a single variable may be declared on a line
- Single-line `if` statements and `while` loops are permitted
- Only functions that may be called are `print()` and `read()`, which are declared in the preamble
- Comments are not pemitted
- Logical operators are not permitted
- Variable declarations all take place at the top of a code block

## Notes

AI Declaration:
Tests were generated with the help fo ChatGPT.
