/*
 * syntax.y - parser for a MiniC program
 *
 * Josh Meise
 * 01-16-2026
 * Description: 
 * - Defines syntax for a MiniC program
 *
 * Citations:
 *
 * Questions:
 * - Can we declare adn define variables on the same line?
 * - Can if statement and while loop conditions just be a single variable (i.e. not necessarily a full expression)?
 * - Can we have void functions?
 * - Should we mandate a return in an int function and allow a return in a void function?
 * - Should we expect return to always be at the bottom of function?
 * - Should we enforce variables being declared at the top of functions in syntax analysis phase?
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
