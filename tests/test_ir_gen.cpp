/*
 * test_ir_gen.cpp - 
 *
 * Josh Meise
 * 02-23-2026
 * Description: 
 *
 */

#include <ir_gen.h>
#include <ast.h>
#include <iostream>

extern int yyparse(void);
extern int yylex_destroy(void);
extern FILE* yyin;
extern astNode* root;

int main(int argc, char** argv) {
    std::string ifile;
    std::string ofile;
    IRGen ir;
    int ret;

    // Check arguments.
    if (argc != 3) {
        std::cerr << "usage: ./test_ir_gen <in_file.c> <out_file.ll>\n";
        return 1;
    }

    // Extract file names.
    ifile = std::string(argv[1]);
    ofile = std::string(argv[2]);

    // Open file.
    if ((yyin = fopen(ifile.c_str(), "r")) == NULL) {
        std::cerr << "Failed to open file.\n";
        return 1;
    }

    ret = yyparse();

    // Clean up.
    fclose(yyin);
    yylex_destroy();

    if (ret != 0) {
        std::cerr << "Parsing failed.\n";
        return 1;
    }

    // Create LLVM IR.
    ir = IRGen(root);

    ir.write_module_to_file(ofile);

    freeNode(root);

    return 0;
}
