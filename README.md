# MiniCCompiler

MiniCCompiler is a compiler for a subset of the C programming language, developed as part of Dartmouth College's COSC 257 class.

## Note to Grader:
Please see the assignment_2 branch for submission. The README in this repository will contain the algorithm for live code analysis.

I have not yet added a call to my optimizer to the main executable. I will do so once I have completed the AST -> LLVM conversion.

I have an executable for the optimizer in the **tests/** directory. This takes as input an unoptimized `.ll` file and outputs an optimized `.ll` file.

## Usage

To run the compiler:
- In **utils/**:
    - `make clean`
    - `make`
- In **execs/**:
    - `make clean`
    - `make`
    - `./compiler <source_code.c>`

To run tests:
- In **utils/**:
    - `make clean`
    - `make`
- In **execs/**:
    - `make clean`
    - `make`
    - `run_all_tests.sh [-v]` where the `-v` flag indicates the desire for verbose output

## Project Structure

- **execs/**: Executable compiler
- **tests/**: Test harnesses and accompanying tests for each phase in the compiler's development
- **utils/**: Parser, lexer, and modules
- **lib/**: Compiled library of object files
- **.github/**: GitHub Actions automated test workflow

## MiniC Syntax Guide

MiniC is a subset of the C programming language. It follows the same syntactic guidelines as C, with a few minor modifications:
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
- Comments are not permitted
- Logical operators are not permitted
- Variable declarations all take place at the top of a code block

Below is an example MiniC program:
```C
extern void print(int);
extern int read(void);

int main(void) {
    int x;
    x = read();
    while (x > 0)
        x = x - 1;
    print(x);
    return 0;
}
```

## Notes

- AI Declaration: Tests were generated with the help of ChatGPT.
