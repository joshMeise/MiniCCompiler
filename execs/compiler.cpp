/*
 * compiler.cpp - driver program for MiniC program.
 *
 * Josh Meise
 * 01-24-2026
 * Description:
 * - Performs syntax analysis on MiniC program, building as AST.
 * - Performs semantic analysis on AST.
 * - Exits if either syntax analysis or semantic analysis fails.
 *
 */

#include <iostream>
#include <cstdlib>
#include <ast.h>
#include <semantic_analysis.h>
#include <optimizer.h>
#include <ir_gen.h>

extern int yyparse(void);
extern int yylex_destroy(void);
extern FILE* yyin;
extern astNode* root;

int main(int argc, char** argv) {
    int ret;
    Optimizer opt;
    IRGen ir;
    LLVMModuleRef m;

    // Check arguments.
    if (argc != 2) {
        std::cout << "usage: ./compiler <infile.c>\n";
        return 1;
    }

    // Open file.
    if ((yyin = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr, "Failed to open file.\n");
        return 1;
    }

    ret = yyparse();

    if (ret != 0) exit(EXIT_FAILURE);

    ret = semantically_analyze(root);

    if (ret != 0) exit(EXIT_FAILURE);

    ir = IRGen(root);

    m = ir.get_module_ref();

    opt = Optimizer(m);

    // Clean up.
    freeNode(root);
    fclose(yyin);
    yylex_destroy();

    exit(EXIT_SUCCESS);
}
