/*
 * assembly_generator.cpp - 
 *
 * Josh Meise
 * 03-03-2026
 * Description: 
 *
 */

#include <iostream>
#include <unordered_set>
#include "assembly_generator.h"

#define NUM_REGS 3

static LLVMValueRef get_last_use(LLVMBasicBlockRef bb, LLVMValueRef inst) {
    LLVMValueRef last_inst;
    LLVMUseRef use, last_use;

    if (bb == NULL || inst == NULL) {
        std::cerr << "Inavlid argument(s) to function.\n";
        return NULL;
    }

    // Find first use of instruction in basic block.
    if ((use = LLVMGetFirstUse(inst)) == NULL) {
        std::cerr << "No use found for instruction.\n";
        return NULL;
    }

    // Walk through all instructions in basic block.
    last_use = NULL;
    while (use != NULL && LLVMGetInstructionParent(LLVMGetUser(use)) == bb) {
        last_use = use;
        use = LLVMGetNextUse(use);
    }

    // Return instruction itself if no uses.
    if (last_use != NULL) {
        if ((last_inst = LLVMGetUser(last_use)) == NULL) {
            std::cerr << "No instruction user found.\n";
            return NULL;
        }
        return last_inst;
    } else {
        return inst;
    }
}

static std::optional<std::unordered_map<LLVMValueRef, int>> get_inst_index(LLVMBasicBlockRef bb) {
    std::unordered_map<LLVMValueRef, int> inst_index;
    LLVMValueRef i;
    int inst_num;

    // Check argument.
    if (bb == NULL) {
        std::cerr << "Invalid argument to function.\n";
        return std::nullopt;
    }

    // Map each instruction to index.
    inst_num = 0;
    for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
        // Ignore alloca instructions.
        if (LLVMGetInstructionOpcode(i) != LLVMAlloca) {
            inst_index[i] = inst_num;
            inst_num++;
        }
    }

    return inst_index;
}

static std::optional<std::unordered_map<LLVMValueRef, std::pair<int, int>>> get_live_range(LLVMBasicBlockRef bb, std::unordered_map<LLVMValueRef, int> inst_index) {
    LLVMValueRef i, last_use;
    std::unordered_map<LLVMValueRef, std::pair<int, int>> live_range;

    // Check arguments.
    if (bb == NULL) {
        std::cerr << "Invalid argument to function.\n";
        return std::nullopt;
    }

    // Get live range for each relevant instruction.
    for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
        // Only deal with instructions which have a left hand side.
        if (LLVMGetInstructionOpcode(i) != LLVMAlloca && LLVMGetTypeKind(LLVMTypeOf(i)) != LLVMVoidTypeKind) {
            if ((last_use = get_last_use(bb, i)) == NULL) {
                std::cerr << "Failed to find last use.\n";
                return std::nullopt;
            }

            live_range[i] = std::make_pair(inst_index[i], inst_index[last_use]);
        }
    }

    return live_range;
}

std::optional<std::unordered_map<LLVMValueRef, int>> allocate_registers(LLVMModuleRef m) {
    LLVMBasicBlockRef bb;
    LLVMValueRef inst, operand;
    LLVMOpcode opcode;
    std::unordered_set<int> avail_regs;
    std::unordered_map<LLVMValueRef, int> reg_map;
    std::unordered_map<LLVMValueRef, int> inst_index;
    std::optional<std::unordered_map<LLVMValueRef, int>> inst_index_opt;
    std::optional<std::unordered_map<LLVMValueRef, std::pair<int, int>>> live_range_opt;
    std::unordered_map<LLVMValueRef, std::pair<int, int>> live_range;
    int i;

    // Check argument.
    if (m == NULL) {
        std::cerr << "Invlaid argument to function.\n";
        return std::nullopt;
    }

    // There will only be one function.
    for (bb = LLVMGetFirstBasicBlock(LLVMGetFirstFunction(m)); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
        // Map instructions to number.
        inst_index_opt = get_inst_index(bb);

        if (!inst_index_opt.has_value()) {
            std::cerr << "Failed to map instructions to indices.\n";
            return std::nullopt;
        } else
            inst_index = inst_index_opt.value();

        // Get live range for each instruction.
        live_range_opt = get_live_range(bb, inst_index);

        if (!live_range_opt.has_value()) {
            std::cerr << "Failed to obtain live ranges.\n";
            return std::nullopt;
        } else
            live_range = live_range_opt.value();

        // Initialize set of available registers.
        for (i = 0; i < NUM_REGS; i++)
            avail_regs.insert(i);

        // Loop through all instructions in basic block.
        for (inst = LLVMGetFirstInstruction(bb); inst != NULL; inst = LLVMGetNextInstruction(inst)) {
            // Instructions that do not have a result.
            if (LLVMGetTypeKind(LLVMTypeOf(inst)) == LLVMVoidTypeKind) {

                // Check all operands and see if live range ends.
                for (i = 0; i < LLVMGetNumOperands(inst); i++) {
                    operand = LLVMGetOperand(inst, i);

                    // Ignore alloca instructions.
                    if (LLVMGetInstructionOpcode(operand) != LLVMAlloca && !LLVMIsAArgument(operand)) {
                        // Check if live range ends.
                        if (live_range[operand].second == inst_index[inst]) {
                            // If operand is currently assigned a register, add the register to the set of available registers.
                            if (reg_map[operand] != -1)
                                avail_regs.insert(reg_map[operand]);
                        }
                    }
                }
            }
            // Instructions that do have a result (ignoring alloca instructions).
            else if (LLVMGetInstructionOpcode(inst) != LLVMAlloca && LLVMGetTypeKind(LLVMTypeOf(inst)) != LLVMVoidTypeKind) {
                opcode = LLVMGetInstructionOpcode(inst);
                // Instructions with two operands.
                // Question: In the algorithm it specifies onluym add, mul and sub but should we be considering lt, gt, etc too?
                if (opcode == LLVMAdd || opcode == LLVMSub || opcode == LLVMMul) {

                }

            }
        }
    }

    return reg_map;
}
