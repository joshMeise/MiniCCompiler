/*
 * ir_gen.cpp - 
 *
 * Josh Meise
 * 02-17-2026
 * Description: 
 *
 * Questins:
 * - Think about return statement having to make a new basic block.
 * - How do stand-alone statements work? How would print() work or a single reference to a variable?
 * - Condition of loop should be able to be 4 + 3 (at the moment it has to be some kind of expression). Do we just want build_ir_helper to contain non-boilerplate stuff and to actually return an LLVMValueRef?
 *
 */

#include "ir_gen.h"
#include <iostream>
#include <unordered_map>
#include <stdexcept>
#include <cstring>
#include <optional>

IRGen::IRGen(void) {
    m = NULL;
    b = NULL;
    func = NULL;
    read_func = NULL;
    read_type = NULL;
    print_func = NULL;
    print_type = NULL;
    ret_alloca = NULL;
    ret_bb = NULL;
    var_num = 0;
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

    // Initialize other members.
    func = NULL;
    read_func = NULL;
    read_type = NULL;
    print_func = NULL;
    print_type = NULL;
    ret_alloca = NULL;
    ret_bb = NULL;
    var_num = 0;

    // Build IR.
    if (build_ir_helper(root) != 0) {
        LLVMDisposeBuilder(b);
        LLVMDisposeModule(m);
        throw std::runtime_error("Failed to build LLVM IR.\n");
    }

    // Free builder.
    LLVMDisposeBuilder(b);
    b = NULL;

}

IRGen::~IRGen(void) {
    if (m != NULL) LLVMDisposeModule(m);
    if (b != NULL) LLVMDisposeBuilder(b);

    m = NULL;
    b = NULL;
    var_to_alloca.clear();
    func = NULL;
    read_func = NULL;
    read_type = NULL;
    print_func = NULL;
    print_type = NULL;
    ret_alloca = NULL;
    ret_bb = NULL;
    var_to_name.clear();
    var_num = 0;

    LLVMShutdown();
}

LLVMModuleRef IRGen::get_module_ref(void) const { return m; }

void IRGen::write_module_to_file(std::string& fname) {
    char* err;

    if (LLVMPrintModuleToFile(m, fname.c_str(), &err) != 0) {
        std::cerr << err << std::endl;
        LLVMDisposeMessage(err);
    }
}

IRGen& IRGen::operator=(IRGen&& other) {
    if (this != &other) {
        // Ensure all is clean in new object; avoid memeory leaks.
        if (m != NULL) LLVMDisposeModule(m);
        if (b != NULL) LLVMDisposeBuilder(b);

        // Copy over values and reset old to a valid initial state.
        m = other.m;
        other.m = NULL;

        b = other.b;
        other.b = NULL;

        var_to_alloca = other.var_to_alloca;
        other.var_to_alloca.clear();

        func = other.func;
        other.func = NULL;

        read_func = other.read_func;
        other.read_func = NULL;

        read_type = other.read_type;
        other.read_type = NULL;

        print_func = other.print_func;
        other.print_func = NULL;

        print_type = other.print_type;
        other.print_type = NULL;

        ret_alloca = other.ret_alloca;
        other.ret_alloca = NULL;

        ret_bb = other.ret_bb;
        other.ret_bb = NULL;

        var_to_name = other.var_to_name;
        other.var_to_name.clear();

        var_num = other.var_num;
        other.var_num = 0;
    }

    return *this;
}

int IRGen::build_ir_helper(astNode* node) {
    LLVMTypeRef ft;
    LLVMBasicBlockRef bb;
    std::vector<LLVMTypeRef> param_types;
    LLVMValueRef ret_val;

    // Check arguments.
    if (node == NULL) {
        std::cerr << "Invalid argument to function.\n";
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

                // Add function to module.
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

            // Create first basic block.
            if ((bb = LLVMAppendBasicBlock(func, "")) == NULL) {
                std::cerr << "Failed to create basic block.\n";
                return -1;
            }

            // Move builder to end of basic block.
            LLVMPositionBuilderAtEnd(b, bb);

            // Create an alloca for the return statement.
            if ((ret_alloca = LLVMBuildAlloca(b, LLVMInt32Type(), "")) == NULL) {
                std::cerr << "Failed to create alloca for return.\n";
                return -1;
            }

            // Resolve variables.
            if (resolve_vars(node) != 0) {
                std::cerr << "Failed to resolve variables.\n";
                return -1;
            }

            // Store parameter into alloca's location.
            if (node->func.param != NULL) {
                if (LLVMBuildStore(b, LLVMGetParam(func, 0), var_to_alloca.find(std::string("v0"))->second) == NULL) {
                    std::cerr << "Failed to build store for parameter.\n";
                    return -1;
                }
            }

            // Create basic block for return.
            if ((ret_bb = LLVMAppendBasicBlock(func, "")) == NULL) {
                std::cerr << "Failed to create basic block.\n";
                return -1;
            }

            // Build IR for body.
            if (build_ir_helper(node->func.body) != 0) {
                std::cerr << "Failed to build IR for body.\n";
                return -1;
            }

            // Move builder to end of return basic block.
            LLVMPositionBuilderAtEnd(b, ret_bb);

            // Load from return's alloca.
            if ((ret_val = LLVMBuildLoad2(b, LLVMInt32Type(), ret_alloca, "")) == NULL) {
                std::cerr << "Failed to build load from return instruction.\n";
                return -1;
            }

            // Create return statement.
            if (LLVMBuildRet(b, ret_val) == NULL) {
                std::cerr << "Failed to build return statement.\n";
                return -1;
            }

            break;
        }
        case ast_stmt: {
            // Analyze statement.
            if (node->stmt.type != ast_call && build_ir_stmt(&(node->stmt)) != 0) {
                std::cerr << "Failed to build IR for statement.\n";
                return -1;
            } else if (node->stmt.type == ast_call && build_ir_expr(node) == NULL) {
                std::cerr << "Failed to build IR for call statement.\n";
                return -1;
            }
            break;
        }
        case ast_var:
        case ast_cnst:
        case ast_rexpr:
        case ast_bexpr:
        case ast_uexpr: {
            if (build_ir_expr(node) == NULL) {
                std::cerr << "Failed to build IR for expression.\n";
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

int IRGen::build_ir_stmt(astStmt* stmt) {
    LLVMValueRef val, rhs, cond_ref, li;
    LLVMBasicBlockRef cond_bb, if_bb, else_bb, while_bb, final_bb, cur_bb;
    std::vector<astNode*>::iterator it;
    std::unordered_map<std::string, LLVMValueRef>::iterator map_it;
    bool found_return;

    // Check arguments.
    if (stmt == NULL) {
        std::cerr << "Invalid argument to function.\n";
        return -1;
    }

    switch (stmt->type) {
        case ast_decl: {
            // Already dealt with during preprocessing.
            break;
        }
        case ast_asgn: {
            // Find LHS variable's alloca statement.
            if ((map_it = var_to_alloca.find(std::string(stmt->asgn.lhs->var.name))) == var_to_alloca.end()) {
                std::cerr << "Could not find alloca for variable.\n";
                return -1;
            }

            // Get value ref for RHS (will always be an expression).
            if ((rhs = build_ir_expr(stmt->asgn.rhs)) == NULL) {
                std::cerr << "Failed to build IR for RHS.\n";
                return -1;
            }

            // Store RHS into LHS.
            if (LLVMBuildStore(b, rhs, map_it->second) == NULL) {
                std::cerr << "Failed to build store instruction.\n";
                return -1;
            }
            break;
        }
        case ast_if: {
            // If curent basic block is empty, make that the considiton basic block.
            if ((cur_bb = LLVMGetInsertBlock(b)) == NULL) {
                std::cerr << "Could not find current basic block.\n";
                return -1;
            }

            if (LLVMGetFirstInstruction(cur_bb) == NULL)
                cond_bb = cur_bb;
            else {
                // Create basic block for condition.
                if ((cond_bb = LLVMInsertBasicBlock(ret_bb, "")) == NULL) {
                    std::cerr << "Failed to create basic block.\n";
                    return -1;
                }

                // Create branch to condition basic block.
                if (LLVMIsATerminatorInst(LLVMGetLastInstruction(cur_bb)) == NULL) {
                    if (LLVMBuildBr(b, cond_bb) == NULL) {
                        std::cerr << "Failed to create unconditional branch instruction.\n";
                        return -1;
                    }
                }
            }

            // Move builder to end of basic block.
            LLVMPositionBuilderAtEnd(b, cond_bb);

            // Build IR for condition (will be an expression).
            if ((cond_ref = build_ir_expr(stmt->ifn.cond)) == NULL) {
                std::cerr << "Failed to genrate IR for condition.\n";
                return -1;
            }

            // Create body basic block.
            if ((if_bb = LLVMInsertBasicBlock(ret_bb, "")) == NULL) {
                std::cerr << "Failed to create basic block.\n";
                return -1;
            }

            // Move builder into body basic lock.
            LLVMPositionBuilderAtEnd(b, if_bb);

            // Build IR for body.
            if (build_ir_helper(stmt->ifn.if_body) != 0) {
                std::cerr << "Failed to build IR for if body.\n";
                return -1;
            }

            // Build out IR for else body if it exists.
            if (stmt->ifn.else_body != NULL) {
                if ((else_bb = LLVMInsertBasicBlock(ret_bb, "")) == NULL) {
                    std::cerr << "Failed to create basic block.\n";
                    return -1;
                }

                // Move builder into else block.
                LLVMPositionBuilderAtEnd(b, else_bb);

                // Build IR for body.
                if (build_ir_helper(stmt->ifn.else_body) != 0) {
                    std::cerr << "Failed to build IR for else body.\n";
                    return -1;
                }
            }

            // Build final basic block if current basic block is not empty.
            if (LLVMGetFirstInstruction(LLVMGetInsertBlock(b)) != NULL) {
                if ((final_bb = LLVMInsertBasicBlock(ret_bb, "")) == NULL) {
                    std::cerr << "Failed to create basic block.\n";
                    return -1;
                }
            } else
            final_bb = LLVMGetInsertBlock(b);

            // Move builder back into condition basic block.
            LLVMPositionBuilderAtEnd(b, cond_bb);

            // Conditional jump instruction to if block, else to else or final block.
            if (stmt->ifn.else_body != NULL) {
                if (LLVMBuildCondBr(b, cond_ref, if_bb, else_bb) == NULL) {
                    std::cerr << "Failed to create conditional branch instruction.\n";
                    return -1;
                }
            } else {
                if (LLVMBuildCondBr(b, cond_ref, if_bb, final_bb) == NULL) {
                    std::cerr << "Failed to create conditional branch instruction.\n";
                    return -1;
                }
            }

            // Move builder to end of if block.
            LLVMPositionBuilderAtEnd(b, if_bb);

            // Create branch to next basic block if last instruction is not a terminator.
            if ((li = LLVMGetLastInstruction(LLVMGetInsertBlock(b))) == NULL || LLVMIsATerminatorInst(li) == NULL) {
                if (LLVMBuildBr(b, final_bb) == NULL) {
                    std::cerr << "Failed to create unconditional branch instruction.\n";
                    return -1;
                }
            }

            if (stmt->ifn.else_body != NULL) {
                // Move builder to end of else block.
                LLVMPositionBuilderAtEnd(b, else_bb);

                // Create branch to next basic block if last statement is not already a terminator (return at end of if statement).
                if ((li = LLVMGetLastInstruction(LLVMGetInsertBlock(b))) == NULL || LLVMIsATerminatorInst(li) == NULL) {
                    if (LLVMBuildBr(b, final_bb) == NULL) {
                        std::cerr << "Failed to create unconditional branch instruction.\n";
                        return -1;
                    }
                }
            }

            // Move builder into final basic block.
            LLVMPositionBuilderAtEnd(b, final_bb);
            break;        }
        case ast_while: {
            // If current basic block is empty, make that the condition basic block.
            if ((cur_bb = LLVMGetInsertBlock(b)) == NULL) {
                std::cerr << "Could not find current basic block.\n";
                return -1;
            }

            if (LLVMGetFirstInstruction(cur_bb) == NULL)
                cond_bb = cur_bb;
            else {
                // Create basic block for condition.
                if ((cond_bb = LLVMInsertBasicBlock(ret_bb, "")) == NULL) {
                    std::cerr << "Failed to create basic block.\n";
                    return -1;
                }

                // Create branch to next basic block.
                if (LLVMIsATerminatorInst(LLVMGetLastInstruction(cur_bb)) == NULL) {
                    if (LLVMBuildBr(b, cond_bb) == NULL) {
                        std::cerr << "Failed to create unconditional branch instruction.\n";
                        return -1;
                    }
                }
            }

            // Move builder to end of basic block.
            LLVMPositionBuilderAtEnd(b, cond_bb);

            // Build IR for condition (will be an expression).
            if ((cond_ref = build_ir_expr(stmt->whilen.cond)) == NULL) {
                std::cerr << "Failed to geenrate IR for condition.\n";
                return -1;
            }

            // Create body basic block.
            if ((while_bb = LLVMInsertBasicBlock(ret_bb, "")) == NULL) {
                std::cerr << "Failed to create basic block.\n";
                return -1;
            }

            // Move builder into body block.
            LLVMPositionBuilderAtEnd(b, while_bb);

            // Build IR for body.
            if (build_ir_helper(stmt->whilen.body) != 0) {
                std::cerr << "Failed to build IR for body.\n";
                return -1;
            }

            // See if last instruction in block is a terminator instruction or if while body block is empty.
            if (((li = LLVMGetLastInstruction(LLVMGetInsertBlock(b))) != NULL && LLVMIsATerminatorInst(li) == NULL) || li == NULL) {
                // Branch back to considiton block.
                if (LLVMBuildBr(b, cond_bb) == NULL) {
                    std::cerr << "Failed to create unconditional branch instruction.\n";
                    return -1;
                }

                // Make final basic block.
                if ((final_bb = LLVMInsertBasicBlock(ret_bb, "")) == NULL) {
                    std::cerr << "Failed to create basic block.\n";
                    return -1;
                }
            } 
            // If last instruction is a terminator instruction, do not create a new final basic block.
            else if ((li = LLVMGetLastInstruction(LLVMGetInsertBlock(b))) != NULL && LLVMIsATerminatorInst(li) != NULL) {
                final_bb = LLVMGetNextBasicBlock(LLVMGetInsertBlock(b));
            }

            // Insert conditional jump instruction to while block, else to final block.
            LLVMPositionBuilderAtEnd(b, cond_bb);
            if (LLVMBuildCondBr(b, cond_ref, while_bb, final_bb) == NULL) {
                std::cerr << "Failed to create conditional branch instruction.\n";
                return -1;
            }

            // Move builder to final basic block.
            LLVMPositionBuilderAtEnd(b, final_bb);

            break;
        }
        case ast_block: {
            // Generate IR for each node.
            found_return = false;
            for (it = stmt->block.stmt_list->begin(); it != stmt->block.stmt_list->end() && !found_return; ++it) {
                if (build_ir_helper(*it) != 0) {
                    std::cerr << "Failed to generate IR for statement.\n";
                    return -1;
                }

                // If this is a return statement, skip over the rest of the block.
                if ((*it)->type == ast_stmt && (*it)->stmt.type == ast_ret)
                    found_return = true;
            }
            break;
        }
        case ast_ret: {
            // Get value reference to return body.
            if ((val = build_ir_expr(stmt->ret.expr)) == NULL) {
                std::cerr << "Failed to build IR for return.\n";
                return -1;
            }

            // Create a store to the return instruction.
            if (LLVMBuildStore(b, val, ret_alloca) == NULL) {
                std::cerr << "Failed to build store for return.\n";
                return -1;
            }

            // Create branch to return basic block.
            if (LLVMBuildBr(b, ret_bb) == NULL) {
                std::cerr << "Failed to create unconditional branch instruction to return.\n";
                return -1;
            }
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

int IRGen::resolve_vars(astNode* node) {
    std::string vname, oname;
    std::unordered_map<std::string, std::string>::iterator it;

    // Ensure that node exists.
    if (node == NULL) {
        std::cerr << "Node does not exist.\n";
        return -1;
    }

    switch(node->type) {
        case ast_prog: {
            // Resolve variables in program body.
            if (resolve_vars(node->prog.func) != 0) {
                std::cerr << "Failed to resolve variables in function.\n";
                return -1;
            }
            break;
        }
        case ast_extern: {
            // No effect on variables.
            break;
        }
        case ast_stmt: {
            // Add map for block statement.
            if (node->stmt.type == ast_block)
                var_to_name.push_back(var_to_name.back());

            if (resolve_vars_stmt_helper(&(node->stmt)) != 0) {
                std::cerr << "Failed to resolve variables in statement.\n";
                return -1;
            }
            break;
        }
        case ast_func: {
            // Add map for function block.
            var_to_name.push_back(std::unordered_map<std::string, std::string>());

            // If function has a parameter, map parameter to new name.
            if (node->func.param != NULL) {
                if (resolve_vars(node->func.param) != 0) {
                    std::cerr << "Failed to resolve parameter.\n";
                    return -1;
                }
            }

            // Resolve for funciton body.
            // Jump straight to statement to avoid adding a new map for function body.
            if (resolve_vars_stmt_helper(&(node->func.body->stmt)) != 0) {
                std::cerr << "Failed to resolve variables in function body.\n";
                return -1;
            }

            break;
        }
        case ast_var: {
            // Replace variable name with new name.
            oname = std::string(node->var.name);

            if ((it = var_to_name.back().find(oname)) == var_to_name.back().end()) {
                std::cerr << "Variable not declared in this scope.\n";
                return -1;
            }

            vname = it->second;

            free(node->var.name);

            if ((node->var.name = (char*)malloc(sizeof(char)*(vname.size() + 1))) == NULL) {
                std::cerr << "Memory allocation failed.\n";
                return -1;
            }

            if (strncpy(node->var.name, vname.c_str(), vname.size()) == NULL) {
                std::cerr << "Failed to copy variable name.\n";
                return -1;
            }

            node->var.name[vname.size()] = '\0';

            break;
        }
        case ast_cnst: {
            // No effect on varaibles.
            break;
        }
        case ast_rexpr: {
            // Resolve variables in both operands.
            if (resolve_vars(node->rexpr.lhs) != 0) {
                std::cerr << "Failed to resolve variables in rexpr.\n";
                return -1;
            }

            if (resolve_vars(node->rexpr.rhs) != 0) {
                std::cerr << "Failed to resolve variables in rexpr.\n";
                return -1;
            }
            break;
        }
        case ast_bexpr: {
            // Resolve variables in both operands.
            if (resolve_vars(node->bexpr.lhs) != 0) {
                std::cerr << "Failed to resolve variables in bexpr.\n";
                return -1;
            }

            if (resolve_vars(node->bexpr.rhs) != 0) {
                std::cerr << "Failed to resolve variables in bexpr.\n";
                return -1;
            }
            break;
        }
        case ast_uexpr: {
            // Resolve variables in operand.
            if (resolve_vars(node->uexpr.expr) != 0) {
                std::cerr << "Failed to resolve variables in uexpr.\n";
                return -1;
            }
            break;
        }
        default: {
            std::cerr << "Unknown node type.\n";
            return -1;
        }
    }

    return 0;
}

int IRGen::resolve_vars_stmt_helper(astStmt* stmt) {
    std::string vname;
    std::vector<astNode*>::iterator it;
    LLVMValueRef val;

    // Check that statement exists.
    if (stmt == NULL) {
        std::cerr << "Statement does not exist.\n";
        return -1;
    }

    switch(stmt->type) {
        case ast_call: {
            // Resolve variables in parameter if contains parameter.
            if (stmt->call.param != NULL) {
                if (resolve_vars(stmt->call.param) != 0) {
                    std::cerr << "Failed to resolve variables in parameter.\n";
                    return -1;
                }
            }
            break;
        }
        case ast_ret: {
            // Resolve variables in return expression.
            if (stmt->ret.expr != NULL) {
                if (resolve_vars(stmt->ret.expr) != 0) {
                    std::cerr << "Failed to resolve variables in return experssion.\n";
                    return -1;
                }
            }
            break;
        }
        case ast_block: {
            // Resolve varibales in all statements.
            for (it = stmt->block.stmt_list->begin(); it != stmt->block.stmt_list->end(); it++) {
                if (resolve_vars(*it) != 0) {
                    std::cerr << "Failed to resolve variables in statement.\n";
                    return -1;
                }
            }

            // Remove block's map.
            var_to_name.pop_back();
            break;
        }
        case ast_while: {
            // Resolve variables in all parts of while statement.
            if (resolve_vars(stmt->whilen.cond) != 0) {
                std::cerr << "Failed to resolve variables in while condition.\n";
                return -1;
            }

            if (resolve_vars(stmt->whilen.body) != 0) {
                std::cerr << "Failed to resolve variables in while body.\n";
                return -1;
            }
            break;
        }
        case ast_if: {
            // Resolve variables in all parts of if statement.
            if (resolve_vars(stmt->ifn.cond) != 0) {
                std::cerr << "Failed to resolve variables in if condition.\n";
                return -1;
            }

            if (resolve_vars(stmt->ifn.if_body) != 0) {
                std::cerr << "Failed to resolve variables in if body.\n";
                return -1;
            }

            if (stmt->ifn.else_body != NULL) {
                if (resolve_vars(stmt->ifn.else_body) != 0) {
                    std::cerr << "Failed to resolve variables in else body.\n";
                    return -1;
                }
            }
            break;
        }
        case ast_asgn: {
            // Resolve variables in both sides.
            if (resolve_vars(stmt->asgn.lhs) != 0) {
                std::cerr << "Failed to resolve variables in assignment.\n";
                return -1;
            }

            if (resolve_vars(stmt->asgn.rhs) != 0) {
                std::cerr << "Failed to resolve variables in assignment.\n";
                return -1;
            }

            break;
        }
        case ast_decl: {
            // Create new variable name.
            vname = std::string("v") + std::to_string(var_num);

            var_num++;

            // Add variable name to map.
            var_to_name.back()[std::string(stmt->decl.name)] = vname;

            // Create alloca for variable.
            if ((val = LLVMBuildAlloca(b, LLVMInt32Type(), "")) == NULL) {
                std::cerr << "Failed to create alloca.\n";
                return -1;
            }

            LLVMSetAlignment(val, 4);

            // Add into map.
            var_to_alloca[vname] = val;

            break;
        }
        default: {
            std::cerr << "Unknown statement type.\n";
            return -1;
        }
    }

    return 0;
}


