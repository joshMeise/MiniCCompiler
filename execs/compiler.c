/*
 * compiler.c - 
 *
 * Josh Meise
 * 01-24-2026
 * Description: 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ast.h>
#include <semantic_analysis.h>

extern int yyparse(void);
extern int yylex_destroy(void);
extern FILE* yyin;
extern astNode* root;

int main(int argc, char** argv) {
    int ret;

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

    ret = yyparse();

    printf("%d\n", semantically_analyze(root));

    // Clean up.
    freeNode(root);
    fclose(yyin);
    yylex_destroy();


    if (ret == 0) exit(EXIT_SUCCESS);
    else exit(EXIT_FAILURE);
}
