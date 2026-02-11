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

/*
 * Given a set of store instructions and an operand, checks to see if there are stores to the same location in the set.
 *
 * Args:
 * - set: set of store instructions.
 * - operand: store location to search for
 *
 * Returns:
 * - set of store instructions to same location as operand
 */
static std::set<LLVMValueRef> find_instrs_with_operand(std::set<LLVMValueRef>& set, LLVMValueRef operand) {
    std::set<LLVMValueRef>::iterator set_it;
    std::set<LLVMValueRef> matches;

    for (set_it = set.begin(); set_it != set.end(); ++set_it)
        if (LLVMGetInstructionOpcode(*set_it) == LLVMStore)
            if (operand == LLVMGetOperand(*set_it, 1))
                matches.insert(*set_it);

    return matches;
}

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
 * Performs global optimizations.
 *
 */
bool Optimizer::global_optimizations(void) {
    LLVMValueRef f;
    LLVMBasicBlockRef bb;
    int i;

    // Map basic blocks to integer values.
    i = 0;
    for (f = LLVMGetFirstFunction(m); f != NULL; f = LLVMGetNextFunction(f))
        for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb))
            lut.insert({bb, i++});

    // Compute GEN, KILL, IN and OUT sets.
    compute_gen_fa();
    compute_kill_fa();
    compute_in_and_out_fa();
    compute_gen_ra();
    compute_kill_ra();
    compute_in_and_out_ra();

    return true;
}

void Optimizer::print_gen_fa(void) {
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>::iterator map_it;
    std::set<LLVMValueRef>::iterator set_it;

    std::cout << "GEN (fa) set:\n";
    for (map_it = gen_fa.begin(); map_it != gen_fa.end(); ++map_it) {
        std::cout << "Block " << lut[map_it->first] << ":\n";
        for (set_it = map_it->second.begin(); set_it != map_it->second.end(); ++set_it) {
            LLVMDumpValue(*set_it);
            std::cout << std::endl;
        }
    }
}

void Optimizer::print_kill_fa(void) {
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>::iterator map_it;
    std::set<LLVMValueRef>::iterator set_it;

    std::cout << "KILL (fa) set:\n";
    for (map_it = kill_fa.begin(); map_it != kill_fa.end(); ++map_it) {
        std::cout << "Block " << lut[map_it->first] << ":\n";
        for (set_it = map_it->second.begin(); set_it != map_it->second.end(); ++set_it) {
            LLVMDumpValue(*set_it);
            std::cout << std::endl;
        }
    }
}

void Optimizer::print_out_fa(void) {
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>::iterator map_it;
    std::set<LLVMValueRef>::iterator set_it;

    std::cout << "OUT (fa) set:\n";
    for (map_it = out_fa.begin(); map_it != out_fa.end(); ++map_it) {
        std::cout << "Block " << lut[map_it->first] << ":\n";
        for (set_it = map_it->second.begin(); set_it != map_it->second.end(); ++set_it) {
            LLVMDumpValue(*set_it);
            std::cout << std::endl;
        }
    }
}

void Optimizer::print_in_fa(void) {
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>::iterator map_it;
    std::set<LLVMValueRef>::iterator set_it;

    std::cout << "IN (fa) set:\n";
    for (map_it = in_fa.begin(); map_it != in_fa.end(); ++map_it) {
        std::cout << "Block " << lut[map_it->first] << ":\n";
        for (set_it = map_it->second.begin(); set_it != map_it->second.end(); ++set_it) {
            LLVMDumpValue(*set_it);
            std::cout << std::endl;
        }
    }
}

void Optimizer::print_gen_ra(void) {
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>::iterator map_it;
    std::set<LLVMValueRef>::iterator set_it;

    std::cout << "GEN (ra) set:\n";
    for (map_it = gen_ra.begin(); map_it != gen_ra.end(); ++map_it) {
        std::cout << "Block " << lut[map_it->first] << ":\n";
        for (set_it = map_it->second.begin(); set_it != map_it->second.end(); ++set_it) {
            LLVMDumpValue(*set_it);
            std::cout << std::endl;
        }
    }
}

void Optimizer::print_kill_ra(void) {
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>::iterator map_it;
    std::set<LLVMValueRef>::iterator set_it;

    std::cout << "KILL (ra) set:\n";
    for (map_it = kill_ra.begin(); map_it != kill_ra.end(); ++map_it) {
        std::cout << "Block " << lut[map_it->first] << ":\n";
        for (set_it = map_it->second.begin(); set_it != map_it->second.end(); ++set_it) {
            LLVMDumpValue(*set_it);
            std::cout << std::endl;
        }
    }
}

void Optimizer::print_out_ra(void) {
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>::iterator map_it;
    std::set<LLVMValueRef>::iterator set_it;

    std::cout << "OUT (ra) set:\n";
    for (map_it = out_ra.begin(); map_it != out_ra.end(); ++map_it) {
        std::cout << "Block " << lut[map_it->first] << ":\n";
        for (set_it = map_it->second.begin(); set_it != map_it->second.end(); ++set_it) {
            LLVMDumpValue(*set_it);
            std::cout << std::endl;
        }
    }
}

void Optimizer::print_in_ra(void) {
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>::iterator map_it;
    std::set<LLVMValueRef>::iterator set_it;

    std::cout << "IN (ra) set:\n";
    for (map_it = in_ra.begin(); map_it != in_ra.end(); ++map_it) {
        std::cout << "Block " << lut[map_it->first] << ":\n";
        for (set_it = map_it->second.begin(); set_it != map_it->second.end(); ++set_it) {
            LLVMDumpValue(*set_it);
            std::cout << std::endl;
        }
    }
}

/*
 * Computes GEN set for all basic blocks of a function using forward.
 * GEN sets take the form of a map from basic block references to sets of instructions.
 */
void Optimizer::compute_gen_fa(void) {
    LLVMValueRef f, i;
    LLVMBasicBlockRef bb;
    std::set<LLVMValueRef> instrs;
    std::set<LLVMValueRef>::iterator set_it;

    // Loop thorugh each basic block and each function.
    for (f = LLVMGetFirstFunction(m); f != NULL; f = LLVMGetNextFunction(f)) {
        for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
            // Create an entry for this basic block.
            gen_fa.insert({bb, std::set<LLVMValueRef>()});

            for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
                // Onlyadd store instructions to GEN set.
                if (LLVMGetInstructionOpcode(i) == LLVMStore) {
                    // Check if store to same operand exists in GEN set.
                    instrs = find_instrs_with_operand(gen_fa[bb], LLVMGetOperand(i, 1));

                    // If an instruction with the given operand already exists in the set, replace it.
                    if (!instrs.empty()) {
                        gen_fa[bb].erase(*instrs.begin());
                        gen_fa[bb].insert(i);
                    }
                    else gen_fa[bb].insert(i);
                }
            }
        }
    }
}

/*
 * Computes KILL set for all basic blocks of a function using forwaard analysis.
 * KILL set takes the form of a map from basic block references to sets of instructions.
 */
void Optimizer::compute_kill_fa(void) {
    LLVMValueRef f, i, loc;
    LLVMBasicBlockRef bb;
    std::set<LLVMValueRef> all_stores;
    std::set<LLVMValueRef>::iterator it;

    // Create set of all store instructions.
    for (f = LLVMGetFirstFunction(m); f != NULL; f = LLVMGetNextFunction(f)) {
        for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
            for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
                if (LLVMGetInstructionOpcode(i) == LLVMStore) all_stores.insert(i);
            }
        }
    }

    // Loop thorugh each basic block and each function.
    for (f = LLVMGetFirstFunction(m); f != NULL; f = LLVMGetNextFunction(f)) {
        for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
            // Create an entry for this basic block.
            kill_fa.insert({bb, std::set<LLVMValueRef>()});

            for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
                // Only add store instructions to KILL set.
                if (LLVMGetInstructionOpcode(i) == LLVMStore) {
                    // Get store location.
                    loc = LLVMGetOperand(i, 1);

                    // Look for store instructuons that are kill_faed.
                    for (it = all_stores.begin(); it != all_stores.end(); ++it) {
                        // A store cannot kill_fa itself.
                        if (*it != i && LLVMGetOperand(*it, 1) == loc) kill_fa[bb].insert(*it);
                    }
                }
            }
        }
    }
}

/*
 * Computes IN and OUT set for all basic blocks of a function using forward analysis.
 * IN and OUT sets take the form of a map from basic block references to sets of instructions.
 */
void Optimizer::compute_in_and_out_fa(void) {
    LLVMValueRef f, term;
    LLVMBasicBlockRef bb, succ;
    std::set<LLVMBasicBlockRef>::iterator bb_it;
    std::set<LLVMValueRef>::iterator val_it;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMBasicBlockRef>> preds;
    bool change;
    unsigned int num;

    // Create empty predecessor serts for eahc block.
    for (f = LLVMGetFirstFunction(m); f != NULL; f = LLVMGetNextFunction(f))
        for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb))
            preds.insert({bb, std::set<LLVMBasicBlockRef>()});

    // Compute predecessor sets for each basic block.
    for (f = LLVMGetFirstFunction(m); f != NULL; f = LLVMGetNextFunction(f)) {
        for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
            // Get terminator for basic block.
            term = LLVMGetBasicBlockTerminator(bb);

            // Add basic block to predecessor set for each of its successors.
            for (num = 0; num < LLVMGetNumSuccessors(term); num++) {
                succ = LLVMGetSuccessor(term, num);
                preds[succ].insert(bb);
            }
        }
    }

    // Initialize IN and OUT sets.
    for (f = LLVMGetFirstFunction(m); f != NULL; f = LLVMGetNextFunction(f)) {
        for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
            // IN sets start out as empty.
            in_fa.insert({bb, std::set<LLVMValueRef>()});

            // OUT set is GEN set of corresponding basic block.
            out_fa.insert({bb, gen_fa.find(bb)->second});
        }
    }

    // Iterate until a fixed point is reached.
    change = true;
    do {
        change = false;
        // First update IN set.
        for (f = LLVMGetFirstFunction(m); f != NULL; f = LLVMGetNextFunction(f)) {
            for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
                // IN set is union of OUT sets of all predecessors.
                for (bb_it = preds[bb].begin(); bb_it != preds[bb].end(); ++bb_it) {
                    for (val_it = out_fa[*bb_it].begin(); val_it != out_fa[*bb_it].end(); ++val_it) {
                        in_fa[bb].insert(*val_it);
                    }
                }
            }
        }

        // Then update OUT set.
        for (f = LLVMGetFirstFunction(m); f != NULL; f = LLVMGetNextFunction(f)) {
            for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
                // Out set contains GEN set of basic block.
                for (val_it = gen_fa[bb].begin(); val_it != gen_fa[bb].end(); ++val_it) {
                    // Check to see if an item is being added to out set.
                    if (!out_fa[bb].contains(*val_it)) {
                        change = true;
                        out_fa[bb].insert(*val_it);
                    }
                }

                // OUT set also contains union of set difference of IN set and KILL set.
                for (val_it = in_fa[bb].begin(); val_it != in_fa[bb].end(); ++val_it) {
                    // Ensure not in KILL set and not already in OUT set.
                    if (!kill_fa[bb].contains(*val_it) && !out_fa[bb].contains(*val_it)) {
                        change = true;
                        out_fa[bb].insert(*val_it);
                    }
                }
            }
        }
    } while (change);
}

/*
 * Computes GEN set for all basic blocks of a function using reverse analysis.
 * GEN sets take the form of a map from basic block references to sets of instructions.
 */
void Optimizer::compute_gen_ra(void) {
    LLVMValueRef f, i;
    LLVMBasicBlockRef bb;
    std::set<LLVMValueRef> stores;

    // Loop thorugh each basic block and each function.
    for (f = LLVMGetFirstFunction(m); f != NULL; f = LLVMGetNextFunction(f)) {
        for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
            // Create set of all store locations in this basic block.
            stores = std::set<LLVMValueRef>();

            // Create an entry for this basic block.
            gen_ra.insert({bb, std::set<LLVMValueRef>()});

            for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
                // Only add load instructions for which no store instructions exist in the same basic block to GEN set.
                if (LLVMGetInstructionOpcode(i) == LLVMLoad && !stores.contains(LLVMGetOperand(i, 0))) gen_ra[bb].insert(i);
                // Add store instructions to set of stores.
                else if (LLVMGetInstructionOpcode(i) == LLVMStore) stores.insert(LLVMGetOperand(i, 1));
            }
        }
    }
}

/*
 * Computes KILL set for all basic blocks of a function using reverse analysis.
 * KILL set takes the form of a map from basic block references to sets of instructions.
 */
void Optimizer::compute_kill_ra(void) {
    LLVMValueRef f, i, loc;
    LLVMBasicBlockRef bb;
    std::set<LLVMValueRef> all_loads;
    std::set<LLVMValueRef>::iterator it;

    // Create set of all loads instructions in program.
    for (f = LLVMGetFirstFunction(m); f != NULL; f = LLVMGetNextFunction(f)) {
        for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
            for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
                if (LLVMGetInstructionOpcode(i) == LLVMLoad) all_loads.insert(i);
            }
        }
    }

    // Loop thorugh each basic block and each function.
    for (f = LLVMGetFirstFunction(m); f != NULL; f = LLVMGetNextFunction(f)) {
        for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
            // Create an entry for this basic block.
            kill_ra.insert({bb, std::set<LLVMValueRef>()});

            for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
                // Only add store instructions to KILL set.
                if (LLVMGetInstructionOpcode(i) == LLVMStore) {
                    // Get store location.
                    loc = LLVMGetOperand(i, 1);

                    // Look for load instructuons that are killed by this store.
                    for (it = all_loads.begin(); it != all_loads.end(); ++it) {
                        if (LLVMGetOperand(*it, 0) == loc) kill_ra[bb].insert(*it);
                    }
                }
            }
        }
    }
}

/*
 * Computes IN and OUT set for all basic blocks of a function using reverse analysis.
 * IN and OUT sets take the form of a map from basic block references to sets of instructions.
 */
void Optimizer::compute_in_and_out_ra(void) {
    LLVMValueRef f, term;
    LLVMBasicBlockRef bb, succ;
    std::set<LLVMBasicBlockRef>::iterator bb_it;
    std::set<LLVMValueRef>::iterator val_it;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMBasicBlockRef>> succs;
    bool change;
    unsigned int num;

    // Compute predecessor sets for each basic block.
    for (f = LLVMGetFirstFunction(m); f != NULL; f = LLVMGetNextFunction(f)) {
        for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
            succs.insert({bb, std::set<LLVMBasicBlockRef>()});

            // Get terminator for basic block.
            term = LLVMGetBasicBlockTerminator(bb);

            // Add basic block to predecessor set for each of its successors.
            for (num = 0; num < LLVMGetNumSuccessors(term); num++) {
                succ = LLVMGetSuccessor(term, num);
                succs[bb].insert(succ);
            }
        }
    }

    // Initialize IN and OUT sets.
    for (f = LLVMGetFirstFunction(m); f != NULL; f = LLVMGetNextFunction(f)) {
        for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
            // OUT sets start out as empty.
            out_ra.insert({bb, std::set<LLVMValueRef>()});

            // IN set is GEN set of corresponding basic block.
            in_ra.insert({bb, gen_ra.find(bb)->second});
        }
    }

    // Iterate until a fixed point is reached.
    change = true;
    do {
        change = false;
        // First update OUT set.
        for (f = LLVMGetFirstFunction(m); f != NULL; f = LLVMGetNextFunction(f)) {
            for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
                // OUT set is union of IN sets of all successors.
                for (bb_it = succs[bb].begin(); bb_it != succs[bb].end(); ++bb_it) {
                    for (val_it = in_ra[*bb_it].begin(); val_it != in_ra[*bb_it].end(); ++val_it) {
                        out_ra[bb].insert(*val_it);
                    }
                }
            }
        }

        // Then update IN set.
        for (f = LLVMGetFirstFunction(m); f != NULL; f = LLVMGetNextFunction(f)) {
            for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
                // IN set contains GEN set of basic block.
                for (val_it = gen_ra[bb].begin(); val_it != gen_ra[bb].end(); ++val_it) {
                    // Check to see if an item is being added to out set.
                    if (!in_ra[bb].contains(*val_it)) {
                        change = true;
                        in_ra[bb].insert(*val_it);
                    }
                }

                // IN set also contains union of set difference of OUT set and KILL set.
                for (val_it = out_ra[bb].begin(); val_it != out_ra[bb].end(); ++val_it) {
                    // Ensure not in KILL set and not already in IN set.
                    if (!kill_ra[bb].contains(*val_it) && !in_ra[bb].contains(*val_it)) {
                        change = true;
                        in_ra[bb].insert(*val_it);
                    }
                }
            }
        }
    } while (change);
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

        if (op == LLVMLoad) {
            operand = LLVMGetOperand(i, 0);
            for (j = LLVMGetNextInstruction(i); j != NULL; j = LLVMGetNextInstruction(j)) {
                std::cout << "HI\n";
            }
        } else {
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
 * Performs constant propagation.
 * Replaces uses of constant load instructions with a constant.
 * Uses sets computed wiht forward analysis.
 */
bool Optimizer::constant_propagation(void) {
    LLVMValueRef f, i;
    LLVMBasicBlockRef bb;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> r;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>::iterator map_it;
    std::set<LLVMValueRef>::iterator set_it;
    bool changes;
    std::set<LLVMValueRef> deletions, stores, killed;

    // Initialize R[B] = IN[B].
    for (map_it = in_fa.begin(); map_it != in_ra.end(); ++map_it)
        r.insert({map_it->first, map_it->second});

    changes = false;
    for (f = LLVMGetFirstFunction(m); f != NULL; f = LLVMGetNextFunction(f)) {
        for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
            for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
                if (LLVMGetInstructionOpcode(i) == LLVMStore) {
                    // Check to see which instructions are killed by i.
                    killed = find_instrs_with_operand(r[bb], LLVMGetOperand(i, 0));

                    // Remove all instructions from R.
                    for (set_it = killed.begin(); set_it != killed.end(); ++set_it) r[bb].erase(*set_it);
                } else if (LLVMGetInstructionOpcode(i) == LLVMLoad) {
                    // Find all stores that store to location of load instruction.
                    stores = find_instrs_with_operand(r[bb], LLVMGetOperand(i, 0));




                }
            }
        }
    }

    // Delete all marked load instructions.
    for (set_it = deletions.begin(); set_it != deletions.end(); ++set_it) LLVMInstructionEraseFromParent(*set_it);

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
