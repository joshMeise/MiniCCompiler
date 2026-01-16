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

program: preamble
       ;

preamble: print read
        ;

print: EXTERN VOID PRINT '(' INT ')' ';'
     ;

read: EXTERN INT READ '(' ')' ';'
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
