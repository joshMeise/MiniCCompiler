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
#include <unordered_map>
#include <set>

class Optimizer {
public:
    Optimizer(void);

    Optimizer(std::string& fname);

    ~Optimizer(void);

    void write_to_file(std::string& fname);

    bool local_optimizations(void);

    bool global_optimizations(void);

    void print_gen(void);

private:
    // Instance variables.
    LLVMModuleRef m;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> gen;

    void compute_gen(void);

    bool common_sub_expr_elim(LLVMBasicBlockRef bb);

    bool dead_code_elim(LLVMBasicBlockRef bb);

    bool constant_folding(LLVMBasicBlockRef bb);
};
