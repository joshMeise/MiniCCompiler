/*
 * ir_gen.h - 
 *
 * Josh Meise
 * 02-17-2026
 * Description: 
 *
 */

#pragma once
#include "ast.h"
#include <llvm-c/Core.h>
#include <unordered_map>
#include <string>
#include <vector>

class IRGen {
public:
    IRGen(void);
    IRGen(astNode* root);
    ~IRGen(void);

    LLVMModuleRef get_module_ref(void) const;
    void write_module_to_file(std::string& fname);

    IRGen& operator=(IRGen&& other);

private:
    LLVMModuleRef m;
    LLVMBuilderRef b;
    std::unordered_map<std::string, LLVMValueRef> var_to_alloca;
    LLVMValueRef func;
    LLVMValueRef read_func;
    LLVMTypeRef read_type;
    LLVMValueRef print_func;
    LLVMTypeRef print_type;
    LLVMValueRef ret_alloca;
    LLVMBasicBlockRef ret_bb;
    std::vector<std::unordered_map<std::string, std::string>> var_to_name;
    int var_num;

    int build_ir_helper(astNode* node);
    int build_ir_stmt(astStmt* stmt);
    LLVMValueRef build_ir_expr(astNode* node);
    int resolve_vars(astNode* node);
    int resolve_vars_stmt_helper(astStmt* stmt);
};
