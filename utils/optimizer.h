/*
 * optimizer.h - 
 *
 * Josh Meise
 * 02-07-2026
 * Description: 
 *
 */

#pragma once
#include <llvm-c/Core.h>
#include <string>

class Optimizer {
public:
    Optimizer(void);

    Optimizer(std::string& fname);

    ~Optimizer(void);

    void write_to_file(std::string& fname);

    bool local_optimizations(void);

private:
    // Instance variables.
    LLVMModuleRef m;

    bool common_sub_expr_elim(LLVMBasicBlockRef bb);

    bool dead_code_elim(LLVMBasicBlockRef bb);

    bool constant_folding(LLVMBasicBlockRef bb);
};
