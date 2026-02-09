/*
 * optimizer.cpp - 
 *
 * Josh Meise
 * 02-07-2026
 * Description: 
 *
 */

#include "optimizer.h"
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#include <exception>
#include <stdexcept>
#include <format>
#include <iostream>
#include <filesystem>

Optimizer::Optimizer(void) {
    m = NULL;
}

Optimizer::Optimizer(std::string& fname) {
    char *err;
    LLVMMemoryBufferRef lmb;

    // Initializations.
    lmb = NULL;
    err = NULL;
    m = NULL;

    // Create LLVM module with file contents.
    if (LLVMCreateMemoryBufferWithContentsOfFile(fname.c_str(), &lmb, &err) != 0) {
        std::cerr << err << std::endl;
        LLVMDisposeMessage(err);
        throw std::runtime_error("Failed to create LLVM memory buffer.\n");
    }

    // Parse the LLVM memory buffer into IR data structures.
    if (LLVMParseIRInContext(LLVMGetGlobalContext(), lmb, &m, &err) != 0) {
        std::cerr << err << std::endl;
        LLVMDisposeMessage(err);
        LLVMDisposeMemoryBuffer(lmb);
        throw std::runtime_error("Failed to parse LLVM memory buffer.\n");
    }
}

Optimizer::~Optimizer(void) {
    if (m != NULL) LLVMDisposeModule(m);
}

void Optimizer::write_to_file(std::string& fname) {
    char *err;

    if (LLVMPrintModuleToFile(m, fname.c_str(), &err) != 0) {
        std::cerr << err << std::endl;
        LLVMDisposeMessage(err);
    }
}

/*
 * Walks through all all basic blocks and performs local optimizations.
 * Optimizations include:
 * - Common subexpression elimination
 *
 * Returns:
 * - bool: true is any changes, false if no changes
 */
bool Optimizer::local_optimizations(void) {
    LLVMValueRef f;
    LLVMBasicBlockRef bb;
    bool sec, dce, cf;

    sec = dce = cf = false;

    // Walk through all basic blocks in each function.
    for (f = LLVMGetFirstFunction(m); f != NULL; f = LLVMGetNextFunction(f)) {
        for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
            //sec = common_sub_expr_elim(bb);
            dce = dead_code_elim(bb);
            cf = constant_folding(bb);
        }
    }

    if (sec)
        return true;
    else
        return false;
}

/*
 * Performs common subexpression elimination.
 * If two instructions have the same opcode and operands with unmodified operands, make the later instruction point to the earlier one.
 *
 * Args:
 * - bb (LLVMBasicBlockRef): pointer to current basic block
 *
 * Returns:
 * - bool: true if any changes, false otherwise
 */
bool Optimizer::common_sub_expr_elim(LLVMBasicBlockRef bb) {
    LLVMValueRef i, j, operand;
    LLVMOpcode op;
    bool changes;

    // Ensure block exists.
    if (bb == NULL) {
        std::cerr << "Invalid argument to common subexpression elimination function.\n";
        return false;
    }

    changes = false;

    // Walk through all instructions in a basic block.
    for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
        op = LLVMGetInstructionOpcode(i);
        LLVMDumpValue(i);
        std::cout << "\n";

        if (op == LLVMLoad) {
            std::cout << "Found load\n";
            operand = LLVMGetOperand(i, 0);
            LLVMDumpValue(operand);
            std::cout << "\n";
            for (j = LLVMGetNextInstruction(i); j != NULL; j = LLVMGetNextInstruction(j)) {
                std::cout << "HI\n";
            }
        } else if (op == LLVMAdd) {
            std::cout << "Found add\n";
            operand = LLVMGetOperand(i, 0);
            LLVMDumpValue(operand);
            std::cout << "\n";
            operand = LLVMGetOperand(i, 1);
            LLVMDumpValue(operand);
            std::cout << "\n";
        }
    }

    return changes;
}

/*
 * Performs dead code elimination.
 * If an instruction is not used, delete that instruction.
 * Does not delete store or return instructions due to possibel side effects or indirect uses.
 *
 * Args:
 * - bb (LLVMBasicBlockRef): pointer to current basic block
 *
 * Returns:
 * - bool: true if any changes, false otherwise
 */
bool Optimizer::dead_code_elim(LLVMBasicBlockRef bb) {
    LLVMValueRef i, prev, next;
    bool changes;

    // Ensure block exists.
    if (bb == NULL) {
        std::cerr << "Invalid argument to common subexpression elimination function.\n";
        return false;
    }

    changes = false;
    prev = NULL;

    for (i = LLVMGetFirstInstruction(bb); i != NULL; i = next) {
        // Instruction is deleted.
        if (LLVMGetFirstUse(i) == NULL && !(LLVMGetInstructionOpcode(i) == LLVMStore || LLVMGetInstructionOpcode(i) == LLVMRet)) {
            // Remove instruction.
            LLVMInstructionEraseFromParent(i);

            // Revert to prevoius instruction.
            if (prev == NULL) next = LLVMGetFirstInstruction(bb);
            else next = LLVMGetNextInstruction(prev);

            // An instruction has been removed by dead code elimination.
            changes = true;
        } 
        // Nothing is deleted.
        else {
            prev = i;
            next = LLVMGetNextInstruction(i);
        }
    }

    return changes;
}

/*
 * Performs constant folding.
 * If an addition, multiplication or subtraction instruction has constant operands, replace uses of that instruction with a constant.
 *
 * Args:
 * - bb (LLVMBasicBlockRef): pointer to current basic block
 *
 * Returns:
 * - bool: true if any changes, false otherwise
 */
bool Optimizer::constant_folding(LLVMBasicBlockRef bb) {
    LLVMValueRef i, lhs, rhs, cnst;
    LLVMOpcode op;
    bool changes;

    // Ensure block exists.
    if (bb == NULL) {
        std::cerr << "Invalid argument to common subexpression elimination function.\n";
        return false;
    }

    changes = false;

    for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
        op = LLVMGetInstructionOpcode(i);

        // Check type of operation.
        if (op == LLVMAdd || op == LLVMSub || op == LLVMMul) {
            // Get LHS and RHS operands.
            lhs = LLVMGetOperand(i, 0);
            rhs = LLVMGetOperand(i, 1);

            // Check to see if operands are constants.
            if (LLVMIsConstant(lhs) && LLVMIsConstant(rhs)) {
                // Conpute constant replacement instructions.
                switch (op) {
                    case LLVMAdd:
                        cnst = LLVMConstAdd(lhs, rhs);
                        break;
                    case LLVMSub:
                        cnst = LLVMConstSub(lhs, rhs);
                        break;
                    case LLVMMul:
                        cnst = LLVMConstMul(lhs, rhs);
                        break;
                    default:
                        std::cerr << "Unrecognized operand.\n";
                }
                // Replace uses of constant add instructions.
                LLVMReplaceAllUsesWith(i, cnst);
                changes = true;
            }
        }
    }
    return changes;
}
