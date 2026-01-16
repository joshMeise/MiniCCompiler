%{

#include <stdio.h>

%}

%start program

%%

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
    if ((yyin = fopen(arg[1], "r")) == NULL) {
        fprintf(stderr, "Failed to open file.\n");
        return 1;
    }
    
    yyparse();

    // Clean up.
    fclose(yyin);
    yylex_destroy();

    return 0;
}
