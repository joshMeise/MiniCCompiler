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
#include <vector>

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

static std::optional<LLVMValueRef> find_spills(LLVMValueRef inst, std::unordered_map<LLVMValueRef, int> reg_map, std::unordered_map<LLVMValueRef, int> inst_index, std::vector<LLVMValueRef> sorted_list, std::unordered_map<LLVMValueRef, std::pair<int, int>> live_range) {
    LLVMValueRef v;

    // Check arguments.
    if (inst == NULL) {
        std::cerr << "Invalid argument to function.\n";
        return std::nullopt;
    }

    return v;
}

std::optional<std::unordered_map<LLVMValueRef, int>> allocate_registers(LLVMModuleRef m) {
    LLVMBasicBlockRef bb;
    LLVMValueRef inst, operand, v;
    LLVMOpcode opcode;
    std::unordered_set<int> avail_regs;
    std::unordered_set<int>::iterator set_it;
    std::unordered_map<LLVMValueRef, int> reg_map;
    std::unordered_map<LLVMValueRef, int> inst_index;
    std::optional<std::unordered_map<LLVMValueRef, int>> inst_index_opt;
    std::optional<std::unordered_map<LLVMValueRef, std::pair<int, int>>> live_range_opt;
    std::unordered_map<LLVMValueRef, std::pair<int, int>> live_range;
    std::optional<LLVMValueRef> opt_v;
    std::vector<LLVMValueRef> sorted_list;
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
                // This is a special case in which a physical register can be saved if first operand has a register.
                // Question: In the algorithm it specifies onluym add, mul and sub but should we be considering lt, gt, etc too? Check assembly to make sense of this.
                if ((opcode == LLVMAdd || opcode == LLVMSub || opcode == LLVMMul) && reg_map[LLVMGetOperand(inst, 0)] != -1 && live_range[LLVMGetOperand(inst, 0)].second == inst_index[inst]) {
                    // Assign instruction register of first operand.
                    reg_map[inst] = reg_map[LLVMGetOperand(inst, 0)];

                    // If live range of second operand ends and it has a register assgined to it, make register available.
                    if (live_range[LLVMGetOperand(inst, 1)].second == inst_index[inst] && reg_map[LLVMGetOperand(inst, 1)] != -1)
                        avail_regs.insert(reg_map[LLVMGetOperand(inst, 1)]);
                }
                // If there is an available pyhsical register.
                else if (!avail_regs.empty()) {
                    // Get a register from set.
                    i = 0;
                    do {
                        // See if register is in available reegisters map.
                        set_it = avail_regs.find(i);
                        i++;
                    } while (i < NUM_REGS && set_it == avail_regs.end());

                    // Mpa instruction to register.
                    reg_map[inst] = *set_it;

                    // Remove regster from set of available registers.
                    avail_regs.erase(*set_it);

                    // Check to see if live range of any operand ends, then add register to list of available registers.
                    for (i = 0; i < LLVMGetNumOperands(inst); i++) {
                        if (live_range[LLVMGetOperand(inst, i)].second == inst_index[inst] && reg_map[LLVMGetOperand(inst, i)] != -1)
                            avail_regs.insert(reg_map[LLVMGetOperand(inst, i)]);
                    }
                }
                // If there are no registers available.
                else if (avail_regs.empty()) {
                    // Compute sorted list.

                    // Find instruction to spill.
                    opt_v = find_spills(inst, reg_map, inst_index, sorted_list, live_range);

                    if (!opt_v.has_value()) {
                        std::cerr << "Failed to find spills.\n";
                        return std::nullopt;
                    } else
                        v = opt_v.value();





                    // Check to see if live range of any operand ends to add its register back to list of available registers.
                    for (i = 0; i < LLVMGetNumOperands(inst); i++) {
                        if (live_range[LLVMGetOperand(inst, i)].second == inst_index[inst] && reg_map[LLVMGetOperand(inst, i)] != -1)
                            avail_regs.insert(reg_map[LLVMGetOperand(inst, i)]);
                    }
                }
            }
        }
    }

    return reg_map;
}
