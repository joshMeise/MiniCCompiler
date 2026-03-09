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
#include <format>

#define NUM_REGS 3

static int get_num_uses(LLVMBasicBlockRef bb, LLVMValueRef inst) {
    LLVMUseRef use;
    int num_uses;

    if (bb == NULL || inst == NULL) {
        std::cerr << "Inavlid argument(s) to function.\n";
        return -1;
    }

    // Find first use of instruction in basic block.
    if ((use = LLVMGetFirstUse(inst)) == NULL)
        return 0;

    num_uses = 1;

    // Walk through all instructions in basic block.
    while (use != NULL && LLVMGetInstructionParent(LLVMGetUser(use)) == bb) {
        use = LLVMGetNextUse(use);
        num_uses++;
    }

    return num_uses;
}

static LLVMValueRef get_last_use(LLVMBasicBlockRef bb, LLVMValueRef inst) {
    LLVMValueRef i;
    int j;
    bool used;

    if (bb == NULL || inst == NULL) {
        std::cerr << "Inavlid argument(s) to function.\n";
        return NULL;
    }

    if (get_num_uses(bb, inst) == 0)
        return inst;

    used = false;
    // Loop through basic block from bottom to find last use of instruction.
    for (i = LLVMGetLastInstruction(bb); i != inst && i != NULL && !used; i = LLVMGetPreviousInstruction(i)) {
        for (j = 0; j < LLVMGetNumOperands(i) && !used; j++) {
            if (LLVMGetOperand(i, j) == inst)
                used = true;
        }
    }

    return LLVMGetNextInstruction(i);
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
    std::vector<LLVMValueRef>::iterator it;
    int vo, vc, io, ic;

    // Check arguments.
    if (inst == NULL) {
        std::cerr << "Invalid argument to function.\n";
        return std::nullopt;
    }

    io = live_range[inst].first;
    ic = live_range[inst].second;

    // Check through every instruction in sorted list.
    for (it = sorted_list.begin(); it != sorted_list.end(); it++) {
        // See if instruction has overlapping liveness with instuction.
        vo = live_range[*it].first;
        vc = live_range[*it].second;
        if (!(vc < io || ic < vo) && reg_map.contains(*it) && reg_map[*it] != -1)
            return *it;
    }

    return std::nullopt;
}
static std::optional<std::unordered_map<LLVMValueRef, int>> get_num_uses_map(LLVMBasicBlockRef bb) {
    std::unordered_map<LLVMValueRef, int> map;
    LLVMValueRef i;

    if (bb == NULL) {
        std::cerr << "Invalid argument to function.\n";
        return std::nullopt;
    }

    for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
        // Only consider non-alloca instructions with return values.
        if (LLVMGetInstructionOpcode(i) != LLVMAlloca && LLVMGetTypeKind(LLVMTypeOf(i)) != LLVMVoidTypeKind) {
            if ((map[i] = get_num_uses(bb, i)) == -1) {
                std::cerr << "Failed to get number of uses.\n";
                return std::nullopt;
            }
        }
    }

    return map;
}

static std::vector<LLVMValueRef> get_sorted_list(std::unordered_map<LLVMValueRef, int> num_uses_map) {
    std::vector<LLVMValueRef> sorted_list;
    LLVMValueRef min_ref;
    std::unordered_map<LLVMValueRef, int>::iterator map_it;
    int min;

    while (!num_uses_map.empty()) {
        // Find minimum number of uses currently in map.
        for (map_it = num_uses_map.begin(); map_it != num_uses_map.end(); map_it++) {
            if (map_it == num_uses_map.begin()) {
                min = map_it->second;
                min_ref = map_it->first;
            } else if (map_it->second < min) {
                min = map_it->second;
                min_ref = map_it->first;
            }
        }

        // Remove minimum element from map and add into sorted list.
        num_uses_map.erase(min_ref);
        sorted_list.push_back(min_ref);
    }

    return sorted_list;
}

static std::optional<std::unordered_map<LLVMBasicBlockRef, std::string>> create_bb_labels(LLVMModuleRef m) {
    LLVMBasicBlockRef bb;
    LLVMValueRef f;
    std::unordered_map<LLVMBasicBlockRef, std::string> labels;
    int num;
    std::string name;

    if (m == NULL) {
        std::cerr << "Invalid argument to function.\n";
        return std::nullopt;
    }

    // There will only be one function with a body.
    if ((f = LLVMGetFirstFunction(m)) == NULL) {
        std::cerr << "Could not find function ref.\n";
        return std::nullopt;
    }

    while (LLVMGetFirstBasicBlock(f) == NULL) {
        if ((f = LLVMGetNextFunction(f)) == NULL) {
            std::cerr << "Could not find function ref.\n";
            return std::nullopt;
        }
    }

    num = 0;
    for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
        name = std::string("BB") + std::to_string(num);
        labels[bb] = name;
        num++;
    }

    return labels;
}

static int print_directives(std::ofstream& ofile) {
    if (!ofile.is_open()) {
        std::cerr << "File not open.\n";
        return -1;
    }

    ofile << "\t.text\n";
    ofile << "\t.globl func\n";
    ofile << "\t.type func, @function\n";
    ofile << "func:\n";

    return 0;
}

static int print_function_end(std::ofstream& ofile) {
    if (!ofile.is_open()) {
        std::cerr << "File not open.\n";
        return -1;
    }

    ofile << "\tleave\n";
    ofile << "\tret\n";

    return 0;
}

static std::optional<std::unordered_map<LLVMValueRef, int>> get_offset_map(LLVMModuleRef m, int& local_mem) {
    std::unordered_map<LLVMValueRef, int> offset_map;
    LLVMValueRef f, param, i;
    LLVMBasicBlockRef bb;
    LLVMOpcode op;

    if (m == NULL) {
        std::cerr << "Invalid argument to funcion.\n";
        return std::nullopt;
    }

    // There will only be one function with a body.
    if ((f = LLVMGetFirstFunction(m)) == NULL) {
        std::cerr << "Could not find function ref.\n";
        return std::nullopt;
    }

    while (LLVMGetFirstBasicBlock(f) == NULL) {
        if ((f = LLVMGetNextFunction(f)) == NULL) {
            std::cerr << "Could not find function ref.\n";
            return std::nullopt;
        }
    }

    local_mem = 4;

    // Add parameter to offset map.
    if ((param = LLVMGetFirstParam(f)) != NULL)
        offset_map[param] = 8;

    for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
        for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
            op = LLVMGetInstructionOpcode(i);
            // Add alloca instructions to offset map.
            if (op == LLVMAlloca) {
                // NB: Added 4 after adding here (differs from algorithm).
                offset_map[i] = -local_mem;
                local_mem += 4;
            }
            else if (op == LLVMStore) {
                if (param != NULL && LLVMGetOperand(i, 0) == param)
                    offset_map[LLVMGetOperand(i, 1)] = offset_map[param];
                else
                    offset_map[LLVMGetOperand(i, 0)] = offset_map[LLVMGetOperand(i, 1)];
            }
            else if (op == LLVMLoad)
                offset_map[i] = offset_map[LLVMGetOperand(i, 0)];
        }
    }

    return offset_map;
}

static std::optional<std::unordered_map<LLVMValueRef, int>> allocate_registers(LLVMModuleRef m) {
    LLVMBasicBlockRef bb;
    LLVMValueRef inst, operand, v, f;
    LLVMOpcode opcode;
    std::unordered_set<int> avail_regs;
    std::unordered_set<int>::iterator set_it;
    std::unordered_map<LLVMValueRef, int> reg_map;
    std::unordered_map<LLVMValueRef, int> inst_index;
    std::optional<std::unordered_map<LLVMValueRef, int>> inst_index_opt;
    std::optional<std::unordered_map<LLVMValueRef, std::pair<int, int>>> live_range_opt;
    std::unordered_map<LLVMValueRef, std::pair<int, int>> live_range;
    std::optional<LLVMValueRef> opt_v;
    std::optional<std::unordered_map<LLVMValueRef, int>> num_uses_map_opt;
    std::unordered_map<LLVMValueRef, int> num_uses_map;
    std::vector<LLVMValueRef> sorted_list;
    int i;

    // Check argument.
    if (m == NULL) {
        std::cerr << "Invlaid argument to function.\n";
        return std::nullopt;
    }

    // There will only be one function with a body.
    if ((f = LLVMGetFirstFunction(m)) == NULL) {
        std::cerr << "Could not find function ref.\n";
        return std::nullopt;
    }

    while (LLVMGetFirstBasicBlock(f) == NULL) {
        if ((f = LLVMGetNextFunction(f)) == NULL) {
            std::cerr << "Could not find function ref.\n";
            return std::nullopt;
        }
    }

    // There will only be one function.
    for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
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

        // Get the number of uses of each instruction.
        num_uses_map_opt = get_num_uses_map(bb);

        if (!num_uses_map_opt.has_value()) {
            std::cerr << "Failed to get num uses map.\n";
            return std::nullopt;
        } else
            num_uses_map = num_uses_map_opt.value();

        // Get sorted list of instructions (sorted by number of uses in ascending order).
        sorted_list = get_sorted_list(num_uses_map);

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
                            if (reg_map.contains(operand) && reg_map[operand] != -1)
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
                if ((opcode == LLVMAdd || opcode == LLVMSub || opcode == LLVMMul) && reg_map.contains(LLVMGetOperand(inst, 0)) && reg_map[LLVMGetOperand(inst, 0)] != -1 && live_range[LLVMGetOperand(inst, 0)].second == inst_index[inst]) {
                    // Assign instruction register of first operand.
                    reg_map[inst] = reg_map[LLVMGetOperand(inst, 0)];

                    // If live range of second operand ends and it has a register assgined to it, make register available.
                    if (live_range[LLVMGetOperand(inst, 1)].second == inst_index[inst] && reg_map.contains(LLVMGetOperand(inst, 1)) && reg_map[LLVMGetOperand(inst, 1)] != -1)
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
                        if (live_range[LLVMGetOperand(inst, i)].second == inst_index[inst] && reg_map.contains(LLVMGetOperand(inst, i)) && reg_map[LLVMGetOperand(inst, i)] != -1)
                            avail_regs.insert(reg_map[LLVMGetOperand(inst, i)]);
                    }
                }
                // If there are no registers available.
                else if (avail_regs.empty()) {
                    // Find instruction to spill.
                    opt_v = find_spills(inst, reg_map, inst_index, sorted_list, live_range);

                    if (!opt_v.has_value()) {
                        std::cerr << "Failed to find spills.\n";
                        return std::nullopt;
                    } else
                        v = opt_v.value();

                    // If spill insturction has more uses than current instruction, do not assign current instruction a register.
                    if (num_uses_map[v] > num_uses_map[inst])
                        reg_map[inst] = -1;
                    // Spill V oherwise.
                    else {
                        reg_map[inst] = reg_map[v];
                        reg_map[v] = -1;
                    }

                    // Check to see if live range of any operand ends to add its register back to list of available registers.
                    for (i = 0; i < LLVMGetNumOperands(inst); i++) {
                        if (live_range[LLVMGetOperand(inst, i)].second == inst_index[inst] && reg_map.contains(LLVMGetOperand(inst, i)) && reg_map[LLVMGetOperand(inst, i)] != -1)
                            avail_regs.insert(reg_map[LLVMGetOperand(inst, i)]);
                    }
                }
            }
        }
    }

    return reg_map;
}

int code_gen(LLVMModuleRef m, std::string fname) {
    std::ofstream ofile;
    std::unordered_map<LLVMBasicBlockRef, std::string> labels;
    std::optional<std::unordered_map<LLVMBasicBlockRef, std::string>> labels_opt;
    std::unordered_map<LLVMValueRef, int> offset_map;
    std::optional<std::unordered_map<LLVMValueRef, int>> offset_map_opt;
    std::unordered_map<LLVMValueRef, int> reg_map;
    std::optional<std::unordered_map<LLVMValueRef, int>> reg_map_opt;
    LLVMValueRef f, i, op1, op2;
    LLVMBasicBlockRef bb;
    int local_mem;
    LLVMOpcode op;
    std::unordered_map<int, std::string> reg;
    std::string r, opr, funcname;
    LLVMIntPredicate pred;

    if (m == NULL) {
        std::cerr << "Invlaid argument to function.\n";
        return -1;
    }

    // Map register numbers to names.
    reg[0] = std::string("%ebx");
    reg[1] = std::string("%ecx");
    reg[2] = std::string("%edx");

    ofile = std::ofstream(fname);

    if (!ofile.is_open()) {
        std::cerr << "Failed to open file.\n";
        return -1;
    }

    // Mpa basic blocks to label names.
    labels_opt = create_bb_labels(m);

    if (!labels_opt.has_value()) {
        std::cerr << "Failed to map basic blocks to labels.\n";
        return -1;
    } else
        labels = labels_opt.value();

    if (print_directives(ofile) != 0) {
        std::cerr << "Failed to print directives.\n";
        return -1;
    }

    // Map instructions to offsets.
    offset_map_opt = get_offset_map(m, local_mem);

    if (!offset_map_opt.has_value()) {
        std::cerr << "Failed to get offset map.\n";
        return -1;
    } else
        offset_map = offset_map_opt.value();

    // Get register map.
    reg_map_opt = allocate_registers(m);

    if (!reg_map_opt.has_value()) {
        std::cerr << "Failed to allocate registers.\n";
        return -1;
    } else
        reg_map = reg_map_opt.value();

    // There will only be one function with a body.
    if ((f = LLVMGetFirstFunction(m)) == NULL) {
        std::cerr << "Could not find function ref.\n";
        return -1;
    }

    while (LLVMGetFirstBasicBlock(f) == NULL) {
        if ((f = LLVMGetNextFunction(f)) == NULL) {
            std::cerr << "Could not find function ref.\n";
            return -1;
        }
    }

    ofile << "\tpushl %ebp\n";
    ofile << "\tmovl %esp, %ebp\n";
    ofile << std::format("\tsubl ${}, %esp\n", local_mem);
    ofile << "\tpushl %ebx\n";

    for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
        // Emit basic block label.
        ofile << labels[bb] << ":" << std::endl;

        for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
            op = LLVMGetInstructionOpcode(i);

            if (op == LLVMRet) {
                op1 = LLVMGetOperand(i, 0);
                if (LLVMIsConstant(op1))
                    ofile << std::format("\tmovl ${}, %eax\n", LLVMConstIntGetSExtValue(op1));
                else if (reg_map[op1] == -1)
                    ofile << std::format("\tmovl {}(%ebp), %eax\n", offset_map[op1]);
                else if (reg_map[op1] != -1)
                    ofile << std::format("\tmovl {}, %eax\n", reg[reg_map[op1]]);

                ofile << "\tpopl %ebx\n";

                if (print_function_end(ofile) != 0) {
                    std::cerr << "Failed to print function postlude.\n";
                    return -1;
                }
            } else if (op == LLVMLoad) {
                if (reg_map[i] != -1)
                    ofile << std::format("\tmovl {}(%ebp), {}\n", offset_map[LLVMGetOperand(i, 0)], reg[reg_map[i]]);
            } else if (op == LLVMStore) {
                op1 = LLVMGetOperand(i, 0);
                op2 = LLVMGetOperand(i, 1);

                // Ignore stores of parameters.
                if (!LLVMIsAArgument(op1) && LLVMIsConstant(op1))
                    ofile << std::format("\tmovl ${}, {}(%ebp)\n", LLVMConstIntGetSExtValue(op1), offset_map[op2]);
                else if (!LLVMIsAArgument(op1) && reg_map[op1] != -1)
                    ofile << std::format("\tmovl {}, {}(%ebp)\n", reg[reg_map[op1]], offset_map[op1]);
                else if (!LLVMIsAArgument(op1) && reg_map[op1] == -1) {
                    ofile << std::format("\tmovl {}(%ebp), %eax\n", offset_map[op1]);
                    ofile << std::format("\tmovl %eax, {}(%ebp)\n", offset_map[op2]);
                }
            } else if (op == LLVMCall) {
                ofile << "\tpushl %ecx\n";
                ofile << "\tpushl %edx\n";

                // Check if function has a parameter.
                if (LLVMGetNumArgOperands(i) != 0) {
                    op1 = LLVMGetArgOperand(i, 0);
                    LLVMDumpValue(op1);
                    std::cout << std::endl;
                    if (LLVMIsConstant(op1))
                        ofile << std::format("\tpushl ${}\n", LLVMConstIntGetSExtValue(op1));
                    else if (reg_map[op1] != -1)
                        ofile << std::format("\tpushl {}\n", reg[reg_map[op1]]);
                    else if (reg_map[op1] == -1)
                        ofile << std::format("\tpushl {}(%ebp)\n", offset_map[op1]);
                    funcname = std::string("print");
                } else
                funcname = std::string("read");

                ofile << std::format("\tcall {}\n", funcname);

                // Undo pushing of parameter.
                if (LLVMGetNumArgOperands(i) != 0)
                    ofile << "\taddl $4, %esp\n";

                ofile << "\tpopl %edx\n";
                ofile << "\tpopl %ecx\n";

                // See if called function returns a value.
                if (LLVMGetTypeKind(LLVMTypeOf(i)) != LLVMVoidTypeKind) {
                    if (reg_map[i] != -1)
                        ofile << std::format("\tmovl %eax, {}\n", reg[reg_map[i]]);
                    else
                        ofile << std::format("\tmovl %eax, {}(%ebp)\n", offset_map[i]);
                }
            } else if (op == LLVMBr) {
                if (LLVMIsConditional(i)) {
                    pred = LLVMGetICmpPredicate(LLVMGetOperand(i, 0));
                    op1 = LLVMGetOperand(i, 2);
                    op2 = LLVMGetOperand(i, 1);

                    // Set jump type.
                    switch (pred) {
                        case LLVMIntEQ:
                            opr = std::string("je");
                            break;
                        case LLVMIntNE:
                            opr = std::string("jne");
                            break;
                        case LLVMIntSGT:
                            opr = std::string("jg");
                            break;
                        case LLVMIntSGE:
                            opr = std::string("jge");
                            break;
                        case LLVMIntSLT:
                            opr = std::string("jl");
                            break;
                        case LLVMIntSLE:
                            opr = std::string("jle");
                            break;
                        default:
                            std::cerr << "Unknown predicate.\n";
                            return -1;
                    }

                    ofile << std::format("\t{} {}\n", opr, labels[LLVMValueAsBasicBlock(op1)]);
                    ofile << std::format("\tjmp {}\n", labels[LLVMValueAsBasicBlock(op2)]);
                } else
                ofile << std::format("\tjmp {}\n", labels[LLVMValueAsBasicBlock(LLVMGetOperand(i, 0))]);
            } else if (op == LLVMAdd || op == LLVMSub || op == LLVMMul) {
                // Check whetehr instruction has a physical register assigned to it.
                if (reg_map[i] == -1)
                    r = std::string("%eax");
                else
                    r = reg[reg_map[i]];

                op1 = LLVMGetOperand(i, 0);
                op2 = LLVMGetOperand(i, 1);

                if (op == LLVMAdd)
                    opr = std::string("addl");
                else if (op == LLVMSub)
                    opr = std::string("subl");
                else if (op == LLVMMul)
                    opr = std::string("imul");

                if (LLVMIsConstant(op1))
                    ofile << std::format("\tmovl ${}, {}\n", LLVMConstIntGetSExtValue(op1), r);
                else if (reg_map[op1] != -1)
                    ofile << std::format("\tmovl {}, {}\n", reg[reg_map[op1]], r);
                else if (reg_map[op1] == -1)
                    ofile << std::format("\tmovl {}(%ebp), {}\n", offset_map[op1], r);

                if (LLVMIsConstant(op2))
                    ofile << std::format("\t{} ${}, {}\n", opr, LLVMConstIntGetSExtValue(op2), r);
                else if (reg_map[op2] != -1)
                    ofile << std::format("\t{} {}, {}\n", opr, reg[reg_map[op2]], r);
                else if (reg_map[op2] == -1)
                    ofile << std::format("\t{} {}(%ebp), {}\n", opr, offset_map[op2], r);

                // If instruction is in memory, move from register to memory.
                if (reg_map[i] == -1)
                    ofile << std::format("\tmovl %eax, {}(%ebp)\n", offset_map[i]);
            } else if (op == LLVMICmp) {
                // Check whetehr instruction has a physical register assigned to it.
                if (reg_map[i] == -1)
                    r = std::string("%eax");
                else
                    r = reg[reg_map[i]];

                op1 = LLVMGetOperand(i, 0);
                op2 = LLVMGetOperand(i, 1);

                if (LLVMIsConstant(op1))
                    ofile << std::format("\tmovl ${}, {}\n", LLVMConstIntGetSExtValue(op1), r);
                else if (reg_map[op1] != -1)
                    ofile << std::format("\tmovl {}, {}\n", reg[reg_map[op1]], r);
                else if (reg_map[op1] == -1)
                    ofile << std::format("\tmovl {}(%ebp), {}\n", offset_map[op1], r);

                if (LLVMIsConstant(op2))
                    ofile << std::format("\tcmpl ${}, {}\n", LLVMConstIntGetSExtValue(op2), r);
                else if (reg_map[op2] != -1)
                    ofile << std::format("\tcmpl {}, {}\n", reg[reg_map[op2]], r);
                else if (reg_map[op2] == -1)
                    ofile << std::format("\tcmpl {}(%ebp), {}\n", offset_map[op2], r);
            } else if (op != LLVMAlloca) {
                std::cerr << "Invalid instruction type.\n";
                return -1;
            }
        }
    }

    ofile << ".func_end:\n";
    ofile << "\t.size func, .func_end-func\n";
    ofile << "\t.section \".note.GNU-stack\", \"\", @progbits\n";

    ofile.close();

    return 0;
}
