/*
 * ir_gen.cpp - 
 *
 * Josh Meise
 * 02-17-2026
 * Description: 
 *
 * Questins:
 * - Does the exit block just remain empty in the case of two consecutive if statements?
 * - How do stand-alone statements work? How would print() work or a single reference to a variable?
 *
 */

#include "ir_gen.h"
#include <iostream>
#include <unordered_map>
#include <stdexcept>

IRGen::IRGen(void) {
    m = NULL;
    b = NULL;
    func = NULL;
    read_func = NULL;
    read_type = NULL;
    print_func = NULL;
    print_type = NULL;
}

IRGen::IRGen(astNode* root) {
    // Check argument.
    if (root == NULL)
        throw std::invalid_argument("Invlid argument to function.\n");

    // Create module.
    if ((m = LLVMModuleCreateWithName("")) == NULL)
        throw std::runtime_error("Failed to create LLVM module.\n");

    LLVMSetTarget(m, "x86_64-pc-linux-gnu");

    // Create builder.
    if ((b = LLVMCreateBuilder()) == NULL)
        throw std::runtime_error("Failed to create LLVM Builder.\n");

    // Build IR.
    if (build_ir_helper(root) != 0) {
        LLVMDisposeBuilder(b);
        LLVMDisposeModule(m);
        throw std::runtime_error("Failed to build LLVM IR.\n");
    }

    // Free builder.
    LLVMDisposeBuilder(b);
    b = NULL;

    // Initialize other members.
    func = NULL;
    read_func = NULL;
    read_type = NULL;
    print_func = NULL;
    print_type = NULL;
}

IRGen::~IRGen(void) {
    if (m != NULL) LLVMDisposeModule(m);
    if (b != NULL) LLVMDisposeBuilder(b);
}

int IRGen::build_ir_helper(astNode* node) {
    LLVMTypeRef ft;
    std::vector<LLVMTypeRef> param_types;

    // Check arguments.
    if (m == NULL || b == NULL || node == NULL) {
        std::cerr << "Invalid argument(s) to function.\n";
        return -1;
    }

    switch (node->type) {
        case ast_prog: {
            // Build out each of the extern functions and the function itself.
            if (node->prog.ext1 != NULL) {
                if (build_ir_helper(node->prog.ext1) != 0) {
                    std::cerr << "Failed to build IR for extern function declaration.\n";
                    return -1;
                }
            }

            if (node->prog.ext2 != NULL) {
                if (build_ir_helper(node->prog.ext2) != 0) {
                    std::cerr << "Failed to build IR for extern function declaration.\n";
                    return -1;
                }
            }

            if (node->prog.func != NULL) {
                if (build_ir_helper(node->prog.func) != 0) {
                    std::cerr << "Failed to build IR for function.\n";
                    return -1;
                }
            }
            break;
        }
        case ast_extern: {
            // Determine whether read print.
            if (std::string(node->ext.name) == "read") {
                // Create read function type.
                if ((read_type = LLVMFunctionType(LLVMInt32Type(), param_types.data(), 0, false)) == NULL) {
                    std::cerr << "Failed to create read function type.\n";
                    return -1;
                }

                // Add function to mpdule.
                if ((read_func = LLVMAddFunction(m, "read", read_type)) == NULL) {
                    std::cerr << "Failed to add read function to module.\n";
                    return -1;
                }
            } else if (std::string(node->ext.name) == "print") {
                // Add integer param type to print function.
                param_types.push_back(LLVMInt32Type());

                // Create print function type.
                if ((print_type = LLVMFunctionType(LLVMVoidType(), param_types.data(), 1, false)) == NULL) {
                    std::cerr << "Failed to create print function type.\n";
                    return -1;
                }

                // Add function to mpdule.
                if ((print_func = LLVMAddFunction(m, "print", print_type)) == NULL) {
                    std::cerr << "Failed to add print function to module.\n";
                    return -1;
                }
            } else {
                std::cerr << "Invalid extern name.\n";
                return -1;
            }
            break;
        }
        case ast_func: {

            // Check if there is a parameter for function.
            if (node->func.param != NULL)
                param_types.push_back(LLVMInt32Type());

            // Create function type which returns an integer.
            if ((ft = LLVMFunctionType(LLVMInt32Type(), param_types.data(), param_types.size(), false)) == NULL) {
                std::cerr << "Failed to create function type.\n";
                return -1;
            }

            // Add function to module.
            if ((func = LLVMAddFunction(m, node->func.name, ft)) == NULL) {
                std::cerr << "Failed to add function to module.\n";
                return -1;
            }

            // Build IR for body.
            if (build_ir_helper(node->func.body) != 0) {
                std::cerr << "Failed to build IR for body.\n";
                return -1;
            }
            break;
        }
        case ast_stmt: {
            // Analyze statement.
            if (build_ir_stmt_helper(&(node->stmt)) != 0) {
                std::cerr << "Failed to build IR for statement.\n";
                return -1;
            }
            break;
        }
        default: {
            std::cerr << "Unrecognized node type.\n";
            return -1;
        }
    }

    return 0;
}

int IRGen::build_ir_stmt_helper(astStmt* stmt) {

    // Check arguments.
    if (m == NULL || b == NULL || stmt == NULL) {
        std::cerr << "Invalid argument(s) to function.\n";
        return -1;
    }

    switch (stmt->type) {
        case ast_decl: {
            break;
        }
        case ast_asgn: {
            break;
        }
        case ast_if: {
            break;
        }
        case ast_while: {
            break;
        }
        case ast_block: {
            break;
        }
        case ast_ret: {
            break;
        }
        default: {
            std::cerr << "Unrecognized statememt type.\n";
            return -1;
        }
    }

    return 0;
}

LLVMValueRef IRGen::build_ir_expr(astNode* node) {
    LLVMValueRef val, op1, op2, zero;
    std::unordered_map<std::string, LLVMValueRef>::iterator it;
    std::vector<LLVMValueRef> args;

    // Check argument.
    if (node == NULL) {
        std::cerr << "Invalid argument to function.\n";
        return NULL;
    }

    switch (node->type) {
        case ast_bexpr: {
            // Call recursevely on both operands.
            if ((op1 = build_ir_expr(node->bexpr.lhs)) == NULL) {
                std::cerr << "Could not build expression for first operand.\n";
                return NULL;
            }

            if ((op2 = build_ir_expr(node->bexpr.rhs)) == NULL) {
                std::cerr << "Could not build expression for second operand.\n";
                return NULL;
            }

            // Create expression depending on operator type.
            switch (node->bexpr.op) {
                case add: {
                    val = LLVMBuildAdd(b, op1, op2, "");
                    break;
                }
                case sub: {
                    val = LLVMBuildSub(b, op1, op2, "");
                    break;
                }
                case mul: {
                    val = LLVMBuildMul(b, op1, op2, "");
                    break;
                }
                default: {
                    std::cerr << "Unrecognized operand type.\n";
                    return NULL;
                }
            }
            break;
        }
        case ast_rexpr: {
            // Call recursevely on both operands.
            if ((op1 = build_ir_expr(node->rexpr.lhs)) == NULL) {
                std::cerr << "Could not build expression for first operand.\n";
                return NULL;
            }

            if ((op2 = build_ir_expr(node->rexpr.rhs)) == NULL) {
                std::cerr << "Could not build expression for second operand.\n";
                return NULL;
            }

            // Create expression depending on operator type.
            switch (node->rexpr.op) {
                case lt: {
                    val = LLVMBuildICmp(b, LLVMIntSLT, op1, op2, "");
                    break;
                }
                case gt: {
                    val = LLVMBuildICmp(b, LLVMIntSGT, op1, op2, "");
                    break;
                }
                case le: {
                    val = LLVMBuildICmp(b, LLVMIntSLE, op1, op2, "");
                    break;
                }
                case ge: {
                    val = LLVMBuildICmp(b, LLVMIntSGE, op1, op2, "");
                    break;
                }
                case eq: {
                    val = LLVMBuildICmp(b, LLVMIntEQ, op1, op2, "");
                    break;
                }
                case neq: {
                    val = LLVMBuildICmp(b, LLVMIntNE, op1, op2, "");
                    break;
                }
                default: {
                    std::cerr << "Unrecognized operand type.\n";
                    return NULL;
                }
            }
            break;
        }
        case ast_uexpr: {
            // Create 0 constant for first operand.
            if ((zero = LLVMConstInt(LLVMInt32Type(), 0, true)) == NULL) {
                std::cerr << "Cound not build value ref for zero.\n";
                return NULL;
            }

            // Call recursively on "second" operand.
            if ((op2 = build_ir_expr(node->uexpr.expr)) == NULL) {
                std::cerr << "Could not build expression for operand.\n";
                return NULL;
            }

            switch (node->uexpr.op) {
                case uminus: {
                    val = LLVMBuildSub(b, zero, op2, "");
                    break;
                }
                default: {
                    std::cerr << "Unrecognized operand type.\n";
                    return NULL;
                }
            }
            break;
        }
        case ast_cnst: {
            // Create vlaue ref for constant value.
            val = LLVMConstInt(LLVMInt32Type(), node->cnst.value, true);
            break;
        }
        case ast_var: {
            // Lookup alloca for variable in map.
            if ((it = var_to_alloca.find(std::string(node->var.name))) == var_to_alloca.end()) {
                std::cerr << "Could not find alloca for variable.\n";
                return NULL;
            }

            // Create load for variable.
            val = LLVMBuildLoad2(b, LLVMInt32Type(), it->second, "");
            break;
        }
        case ast_stmt: {
            if (node->stmt.type != ast_call) {
                std::cerr << "Invalid statement type.\n";
                return NULL;
            }

            if (std::string(node->stmt.call.name) == "read") {
                if (read_type == NULL || read_func == NULL) {
                    std::cerr << "extern int read(void) not declared.\n";
                    return NULL;
                }

                // Create call instruction to read function.
                val = LLVMBuildCall2(b, read_type, read_func, NULL, 0, "");
            } else if (std::string(node->stmt.call.name) == "print") {
                if (print_type == NULL || print_func == NULL) {
                    std::cerr << "extern void print(int) not declared.\n";
                    return NULL;
                }

                // Create a value ref for argument.
                args.push_back(build_ir_expr(node->stmt.call.param));

                // Create call instruction to print function.
                val = LLVMBuildCall2(b, print_type, print_func, args.data(), 1, "");
            } else {
                std::cerr << "Invalid function call.\n";
                return NULL;
            }
            break;
        }
        default: {
            std::cerr << "Invalid type.\n";
            return  NULL;
        }
    }

    return val;
}

