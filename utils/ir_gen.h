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

class IRGen {
public:
    IRGen(void);
    IRGen(astNode* root);
    ~IRGen(void);

    LLVMModuleRef get_module_ref(void) const;

private:
    LLVMModuleRef m;
    LLVMBuilderRef b;
    std::unordered_map<std::string, LLVMValueRef> var_to_alloca;
    LLVMValueRef func;
    LLVMValueRef read_func;
    LLVMTypeRef read_type;
    LLVMValueRef print_func;
    LLVMTypeRef print_type;

    int build_ir_helper(astNode* node);
    int build_ir_stmt_helper(astStmt* stmt);
    LLVMValueRef build_ir_expr(astNode* node);

};
