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

    void print_gen_fa(void);
    void print_kill_fa(void);
    void print_in_fa(void);
    void print_out_fa(void);

    void print_gen_ra(void);
    void print_kill_ra(void);
    void print_in_ra(void);
    void print_out_ra(void);

private:
    // Instance variables.
    LLVMModuleRef m;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> gen_fa;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> kill_fa;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> in_fa;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> out_fa;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> gen_ra;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> kill_ra;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> in_ra;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> out_ra;

    void compute_gen_fa(void);
    void compute_kill_fa(void);
    void compute_in_and_out_fa(void);

    void compute_gen_ra(void);
    void compute_kill_ra(void);
    void compute_in_and_out_ra(void);

    bool common_sub_expr_elim(LLVMBasicBlockRef bb);

    bool dead_code_elim(LLVMBasicBlockRef bb);

    bool constant_folding(LLVMBasicBlockRef bb);
};
