# MiniCCompiler

MiniCCompiler is a compiler for a subset of the C programming language, developed as part of Dartmouth College's COSC 257 class.

## Note to Grader:
Please see the assignment_2 branch for submission. The README in this contains the algorithm for live code analysis.

I have not yet added a call to my optimizer to the main executable. I will do so once I have completed the AST -> LLVM conversion.

I have an executable for the optimizer in the **tests/** directory. This takes as input an unoptimized `.ll` file and outputs an optimized `.ll` file.

## Live Variable Analysis

### Computing GEN sets:
- For each basic block, B:
    - Create an empty GEN set, G
    - Create empty set to hold all stores, S
    - For each instruction, I:
        - If I is a store instruction, add I to S
        - If I is a load instruction and I does not load from a location to which an instruction in S stores, add I to G

### Computing KILL sets:
- Create a set of all loads in the function, L
- For each basic block, B:
    - Create an empty KILL set, K
    - For each instruction, I:
        - If I is a store instruction, add all loads from L to same location as I to K

### Computing IN and OUT sets:
- For each basic block, B
    - Initialize IN[B] to GEN[B]
    - Initialize OUT[B] to empty set
- change = true
- while (change) do:
    - change = false
    - For each basic block, B:
        - OUT[B] = union of IN sets of all successors of B
    - OLD_IN = IN
    - For each basic block, B:
        - IN[B] = union of GEN[B] and set difference of OUT[B] and KILL[B]
    - if OLD_IN != IN, change = true

### Store elimination:
- For each basic block, B:
    - Compute set of all loads, L
- For each basic block, B:
    - For each instruction, I:
    - If I is a load:
        - Remove I from L
    - If I is a store:
        - If there are no loads in L which loads from the location that I stores to and there are no loads in OUT[B] that load from the same location that I stores to:
            - Mark I to be deleted
- Delete all marked instructions

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
