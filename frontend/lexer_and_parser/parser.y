/*
 * syntax.y - parser for a MiniC program
 *
 * Josh Meise
 * 01-16-2026
 * Description: 
 * - Defines syntax for a MiniC program.
 *
 * Assumptions:
 * - All variables are decleared at the top of functions.
 * - Can have unused relational expressions in functions.
 * - Can return relational expressions.
 * - Can have any nesting of parentheses.
 * - Can have standalone expressions inside of code body.
 * - Only functions that can be called are read() and print().
 * - else statements match with innermost if statement unless otehrwise specified with braces.
 * 
 * Citations:
 *
 * Questions:
 * - Can we declare and define variables on the same line?
 *      - Declarations and definitions cannot be combined.
 * - Can if statement and while loop conditions just be a single variable (i.e. not necessarily a full expression)?
 *      - Can be a single variable. Can just have a single term.
 * - Can we have void functions?
 *      - No void functions.
 * - Should we expect return to always be at the bottom of function?
 *      - No. Returns may be treated like any other statement.
 *
 * TODO:
 * - Possibly (void); maybe define rule that allows open or word "void"
 * - Check semi-olons at the edn of statements.
 *
 */

%{

#include <stdio.h>
#include <ast.h>

extern int yylex(void);
extern int linenum;

int yyerror(char*);

%}

%token INT VOID EXTERN PRINT READ IF ELSE WHILE RETURN PLUS MINUS TIMES DIVIDE EQUALS LT GT LEQ GEQ EQ NUMBER IDENTIFIER

%start program

/* Match else statement with innermost if statement in dangling else case. */
%nonassoc NOELSE
%nonassoc ELSE

%%

program: preamble function
       ;

preamble: print read
        ;

print: EXTERN VOID PRINT '(' INT ')' ';'
     ;

read: EXTERN INT READ '(' no_args ')' ';'
    ;

function: return_type function_name argument code_block
        ;

return_type: INT
           | VOID
           ;

function_name: IDENTIFIER
             ;

argument: '(' argument ')'
        | '(' argument_body ')'
        ;

argument_body: INT IDENTIFIER
             | no_args
             ;

code_block: '{' code_block '}'
          | '{' code_block_body '}'
          ;

code_block_body: variable_decs stmts_and_exprs
               ;

variable_decs: variable_dec variable_decs
             | /* empty */
             ;

variable_dec: INT IDENTIFIER ';'
            ;

stmts_and_exprs: stmt_or_expr stmts_and_exprs
               | /* empty */
               ;

stmt_or_expr: stmt
            | expr
            ;

stmt: assignment_stmt ';'
    | while_stmt
    | if_stmt
    | return_stmt ';'
    ;

while_stmt: WHILE condition code_block
          | WHILE condition stmt_or_expr
          ;

if_stmt: if_part else_part
       ;

if_part: IF condition code_block
       | IF condition stmt_or_expr
       ;

else_part: ELSE code_block
         | ELSE stmt_or_expr
         | /* empty */ %prec NOELSE
         ;

return_stmt: RETURN return
           ;

return: '(' return ')'
      | return_body
      ;

return_body: arithmetic_expr
           | relational_expr
           | NUMBER
           | IDENTIFIER
           ;

condition: '(' condition ')'
         | '(' condition_body ')'
         ;

condition_body: relational_expr
              | arithmetic_expr
              | read_expr
              | IDENTIFIER
              | NUMBER
              ;

expr: assignment_expr
    | read_expr ';'
    | print_expr ';'
    ;

assignment_stmt: IDENTIFIER EQUALS assignment_expr
               ;

assignment_expr: arithmetic_expr
               | relational_expr
               | read_expr 
               | IDENTIFIER
               | NUMBER
               ;

arithmetic_expr: plus_expr
               | minus_expr
               | times_expr
               | divide_expr
               ;

plus_expr: expr_arg PLUS expr_arg
         ;

minus_expr: expr_arg MINUS expr_arg
          ;

times_expr: expr_arg TIMES expr_arg
          ;

divide_expr: expr_arg DIVIDE expr_arg
           ;

relational_expr: lt_expr
               | gt_expr
               | leq_expr
               | geq_expr
               | eq_expr
               ;

lt_expr: expr_arg LT expr_arg
       ;

gt_expr: expr_arg GT expr_arg
       ;

leq_expr: expr_arg LEQ expr_arg
        ;

geq_expr: expr_arg GEQ expr_arg
        ;

eq_expr: expr_arg EQ expr_arg
       ;

expr_arg: read_expr
        | IDENTIFIER
        | NUMBER
        ;

read_expr: READ read_arg
         ;

read_arg: '(' read_arg ')'
        | '(' /* empty */ ')'
        ;

print_expr: PRINT print_arg
          ;

print_arg: '(' print_arg ')'
         | '(' assignment_expr ')'
         ;

no_args: VOID
       | /* empty */
       ;

%%

int yyerror(char* s) {
    fprintf(stderr, "%s on line %d\n", s, linenum);
    return 1;
}
