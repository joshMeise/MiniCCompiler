/*
 * optimizer.h - optimizer for MiniC program
 *
 * Josh Meise
 * 02-07-2026
 * Description:
 * Contains functions to perform:
 * - common subexpression elimination
 * - dead code elimination
 * - constant folding
 * - constants propagation
 * - live variable analysis
 *
 * Citations:
 * - ChatGPT for help with move assignment (to avoid segfault in main)
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
    Optimizer(LLVMModuleRef m);
    ~Optimizer(void);
    Optimizer& operator=(Optimizer&& other);

    void write_to_file(std::string& fname);

    int optimize(void);

private:
    // Instance variables.
    LLVMModuleRef m;

    bool common_sub_expr_elim(LLVMBasicBlockRef bb);

    bool dead_code_elim(LLVMBasicBlockRef bb);

    bool constant_folding(LLVMBasicBlockRef bb);

    bool constant_propagation(LLVMValueRef f);

    int live_variable_analysis(LLVMValueRef f);

    void print_set(std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>& print_set);
};
