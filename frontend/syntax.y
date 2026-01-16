/*
 * syntax.y - parser for a MiniC program
 *
 * Josh Meise
 * 01-16-2026
 * Description: 
 * - Defines syntax for a MiniC program.
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
 *
 */

%{

#include <stdio.h>
extern int yylex(void);
extern int yylex_destroy(void);
extern int yywrap(void);
int yyerror(char*);
extern FILE *yyin;

%}

%token INT VOID EXTERN PRINT READ IF ELSE WHILE RETURN PLUS MINUS TIMES DIVIDE EQUALS LT GT LEQ GEQ EQ NUMBER IDENTIFIER

%start program

%%

program: preamble function
       ;

preamble: print read
        ;

print: EXTERN VOID PRINT '(' INT ')' ';'
     ;

read: EXTERN INT READ '(' ')' ';'
    ;

function: return_type function_name '(' argument ')' '{' function_body '}'
        ;

return_type: INT
           | VOID
           ;

function_name: IDENTIFIER
             ;

argument: INT IDENTIFIER
        | /* empty */
        ;

function_body: variable_decs
             ;

variable_decs: variable_dec variable_decs
             | /* empty */
             ;

variable_dec: INT IDENTIFIER ';'
            ;

arithmetic_expr: plus_expr
               | minus_expr
               | times_expr
               | divide_expr
               ;

plus_expr: IDENTIFIER PLUS IDENTIFIER
         | IDENTIFIER PLUS NUMBER
         | NUMBER PLUS IDENTIFIER
         ;

minus_expr: IDENTIFIER MINUS IDENTIFIER
          | IDENTIFIER MINUS NUMBER
          | NUMBER MINUS IDENTIFIER
          ;

times_expr: IDENTIFIER TIMES IDENTIFIER
          | IDENTIFIER TIMES NUMBER
          | NUMBER TIMES IDENTIFIER
          ;

divide_expr: IDENTIFIER DIVIDE IDENTIFIER
           | IDENTIFIER DIVIDE NUMBER
           | NUMBER DIVIDE IDENTIFIER
           ;

relational_expr: lt_expr
               | gt_expr
               | leq_expr
               | geq_expr
               | eq_expr
               ;

lt_expr: IDENTIFIER LT IDENTIFIER
       | IDENTIFIER LT NUMBER
       | NUMBER LT IDENTIFIER
       ;

gt_expr: IDENTIFIER GT IDENTIFIER
       | IDENTIFIER GT NUMBER
       | NUMBER GT IDENTIFIER
       ;

leq_expr: IDENTIFIER LEQ IDENTIFIER
        | IDENTIFIER LEQ NUMBER
        | NUMBER LEQ IDENTIFIER
        ;

geq_expr: IDENTIFIER GEQ IDENTIFIER
        | IDENTIFIER GEQ NUMBER
        | NUMBER GEQ IDENTIFIER
        ;

eq_expr: IDENTIFIER EQ IDENTIFIER
       | IDENTIFIER EQ NUMBER
       | NUMBER EQ IDENTIFIER
       ;


%%

int yyerror(char* s) {
    fprintf(stderr, "%s\n", s);
    return 0;
}

int main(int argc, char** argv) {
    // Check arguments.
    if (argc != 2) {
        fprintf(stderr, "No program provided.\n");
        return 1;
    }

    // Open file.
    if ((yyin = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr, "Failed to open file.\n");
        return 1;
    }
    
    yyparse();

    // Clean up.
    fclose(yyin);
    yylex_destroy();

    return 0;
}
