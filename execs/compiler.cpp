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
#include <assembly_generator.h>

extern int yyparse(void);
extern int yylex_destroy(void);
extern FILE* yyin;
extern astNode* root;

int main(int argc, char** argv) {
    int ret;
    Optimizer opt;
    IRGen ir;
    LLVMModuleRef m;
    std::string ofile;

    // Check arguments.
    if (argc != 3) {
        std::cout << "usage: ./compiler <in_file.c> <out_file.s>\n";
        return 1;
    }

    ofile = std::string(argv[2]);

    // Open file.
    if ((yyin = fopen(argv[1], "r")) == NULL) {
        std::cerr << "Failed to open file.\n";
        return 1;
    }

    ret = yyparse();

    if (ret != 0) {
        std::cerr << "Syntax analysis failed.\n";
        return 1;
    }

    ret = semantically_analyze(root);

    if (ret != 0) {
        std::cerr << "Semantic analysis failed.\n";
        return 1;
    }

    ir = IRGen(root);

    m = ir.get_module_ref();

    opt = Optimizer(m);

    m = opt.get_module_ref();

    if (code_gen(m, ofile) != 0) {
        std::cerr << "Code gen failed.\n";
        return 1;
    }

    // Clean up.
    freeNode(root);
    fclose(yyin);
    yylex_destroy();

    return 0;
}
