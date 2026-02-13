/*
 * optimizer.cpp - optimizer for MiniC program
 *
 * Josh Meise
 * 02-07-2026
 * Description:
 * Contains functions to perform:
 * - common subexpression elimination
 * - dead code elimination
 * - constant folding
 * - constants propagation
 * - live variable analysis
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
#include <optional>

/*
 * Given a set of load or storeinstructions and an operand, checks to see if there are loads or stores to the same location in the set.
 *
 * Args:
 * - set: set of load or store instructions.
 * - operand: location to search for
 *
 * Returns:
 * - Set of instructions to same location as operand
 */
static std::set<LLVMValueRef> find_instrs_with_operand(std::set<LLVMValueRef>& set, LLVMValueRef operand) {
    std::set<LLVMValueRef>::iterator set_it;
    std::set<LLVMValueRef> matches;

    for (set_it = set.begin(); set_it != set.end(); ++set_it) {
        if (LLVMGetInstructionOpcode(*set_it) == LLVMStore) {
            if (operand == LLVMGetOperand(*set_it, 1)) matches.insert(*set_it);
        } else if (LLVMGetInstructionOpcode(*set_it) == LLVMLoad) {
            if (operand == LLVMGetOperand(*set_it, 0)) matches.insert(*set_it);
        }
    }

    return matches;
}

/*
 * Computes GEN set for all basic blocks of a function using forward analysd analysis.
 * GEN sets take the form of a map from basic block references to sets of instructions.
 *
 * Args:
 * - f: function for which to compute set.
 *
 * Returns:
 * - Computed GEN set, nullopt if failure
 */
static std::optional<std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>> compute_gen_fa(LLVMValueRef f) {
    LLVMValueRef i;
    LLVMBasicBlockRef bb;
    std::set<LLVMValueRef> instrs;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> gen_fa;

    if (f == NULL) {
        std::cerr << "Invalid argument to function.\n";
        return std::nullopt;
    }

    for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
        // Create an empty gen set for basic block.
        gen_fa.insert({bb, std::set<LLVMValueRef>()});

        for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
            // Only add store instructions to GEN set.
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

    return gen_fa;
}

/*
 * Computes KILL set for all basic blocks of a function using forwaard analysis.
 * KILL set takes the form of a map from basic block references to sets of instructions.
 *
 * Args:
 * - f: function for which to compute set.
 *
 * Returns:
 * - Computed KILL set, nullopt if failure.
 */
static std::optional<std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>> compute_kill_fa(LLVMValueRef f) {
    LLVMValueRef i, loc;
    LLVMBasicBlockRef bb;
    std::set<LLVMValueRef> all_stores;
    std::set<LLVMValueRef>::iterator set_it;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> kill_fa;

    if (f == NULL) {
        std::cerr << "Invalid argument to function.\n";
        return std::nullopt;
    }

    // Create set of all store instructions.
    for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
        for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
            if (LLVMGetInstructionOpcode(i) == LLVMStore) all_stores.insert(i);
        }
    }

    for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
        // Kill set for each basic block starts off as empty set.
        kill_fa.insert({bb, std::set<LLVMValueRef>()});

        for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
            // Only add store instructions to KILL set.
            if (LLVMGetInstructionOpcode(i) == LLVMStore) {
                // Get store location.
                loc = LLVMGetOperand(i, 1);

                // Look for store instructions that are killed.
                for (set_it = all_stores.begin(); set_it != all_stores.end(); ++set_it) {
                    // A store cannot kill itself.
                    if (*set_it != i && LLVMGetOperand(*set_it, 1) == loc) kill_fa[bb].insert(*set_it);
                }
            }
        }
    }

    return kill_fa;
}

/*
 * Computes IN and OUT set for all basic blocks of a function using forward analysis.
 * IN and OUT sets take the form of a map from basic block references to sets of instructions.
 *
 * Args:
 * - f: function for which to compute set.
 * - gen_fa: GEN set for correspinding function.
 * - kill_fa: KILL set for corresponding function.
 *
 * Returns:
 * - Pair of computed in and out sets, nullopt if failure.
 */
static std::optional<std::pair<std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>>> compute_in_and_out_fa(LLVMValueRef f, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>& gen_fa, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>& kill_fa) {
    LLVMValueRef term;
    LLVMBasicBlockRef bb, succ;
    std::set<LLVMBasicBlockRef>::iterator bb_it;
    std::set<LLVMValueRef>::iterator val_it;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMBasicBlockRef>> preds;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> old_out, in_fa, out_fa;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>::iterator map_it;
    bool change;
    unsigned int num;

    if (f == NULL) {
        std::cerr << "Invalid argument to function.\n";
        return std::nullopt;
    }

    // Create empty predecessor serts for eahc block.
    for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb))
        preds.insert({bb, std::set<LLVMBasicBlockRef>()});

    // Compute predecessor sets for each basic block.
    for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
        // Get terminator for basic block.
        term = LLVMGetBasicBlockTerminator(bb);
        // Add basic block to predecessor set for each of its successors.
        for (num = 0; num < LLVMGetNumSuccessors(term); num++) {
            succ = LLVMGetSuccessor(term, num);
            preds[succ].insert(bb);
        }
    }

    // Initialize IN and OUT sets.
    for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
        // IN sets start out as empty.
        in_fa.insert({bb, std::set<LLVMValueRef>()});

        // OUT set is GEN set of corresponding basic block.
        out_fa.insert({bb, gen_fa.find(bb)->second});
    }

    // Iterate until a fixed point is reached.
    do {
        // First update IN set.
        for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
            // IN set is union of OUT sets of all predecessors.
            for (bb_it = preds[bb].begin(); bb_it != preds[bb].end(); ++bb_it) {
                for (val_it = out_fa[*bb_it].begin(); val_it != out_fa[*bb_it].end(); ++val_it) {
                    in_fa[bb].insert(*val_it);
                }
            }
        }

        // Then update OUT set.
        old_out = out_fa;
        for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
            // Start off with empty out set.
            out_fa[bb] = std::set<LLVMValueRef>();

            // Out set contains GEN set of basic block.
            for (val_it = gen_fa[bb].begin(); val_it != gen_fa[bb].end(); ++val_it) out_fa[bb].insert(*val_it);

            // OUT set also contains union of set difference of IN set and KILL set.
            for (val_it = in_fa[bb].begin(); val_it != in_fa[bb].end(); ++val_it)
                // Ensure not in KILL set and not already in OUT set.
                if (!kill_fa[bb].contains(*val_it)) out_fa[bb].insert(*val_it);
        }

        // Check to see if out set has changed.
        change = false;
        for (map_it = old_out.begin(); map_it != old_out.end() && !change; ++map_it) {
            // Check if old out and new out have the same number of statements.
            if (map_it->second.size() != out_fa[map_it->first].size()) change = true;
            for (val_it = map_it->second.begin(); val_it != map_it->second.end() && !change; ++val_it)
                // Ensure that all elements in old out are in new out.
                if (!out_fa[map_it->first].contains(*val_it)) change = true;
        }

    } while (change);

    return std::make_pair(in_fa, out_fa);
}

/*
 * Computes GEN set for all basic blocks of a function using reverse analysis.
 * GEN sets take the form of a map from basic block references to sets of instructions.
 *
 * Args:
 * - f (LLVMValueRef): function for which to compute set.
 *
 * Returns:
 * - Computed GEN set, nullopt if failure
 */
static std::optional<std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>> compute_gen_ra(LLVMValueRef f) {
    LLVMValueRef i;
    LLVMBasicBlockRef bb;
    std::set<LLVMValueRef> stores;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> gen_ra;

    if (f == NULL) {
        std::cerr << "Invalid argument to function.\n";
        return std::nullopt;
    }

    for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
        // Create set of all store locations in this basic block.
        stores = std::set<LLVMValueRef>();

        // Create an empty gen set for this basic block.
        gen_ra.insert({bb, std::set<LLVMValueRef>()});

        for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i))
            // Only add load instructions for which no store instructions exist in the same basic block to GEN set.
            if (LLVMGetInstructionOpcode(i) == LLVMLoad && !stores.contains(LLVMGetOperand(i, 0))) gen_ra[bb].insert(i);
            // Add store instructions to set of stores.
            else if (LLVMGetInstructionOpcode(i) == LLVMStore) stores.insert(LLVMGetOperand(i, 1));
    }

    return gen_ra;
}

/*
 * Computes KILL set for all basic blocks of a function using reverse analysis.
 * KILL set takes the form of a map from basic block references to sets of instructions.
 *
 * Args:
 * - f (LLVMValueRef): function for which to compute set.
 *
 * Returns:
 * - Computed KILL set, nullopt if failure
 */
static std::optional<std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>> compute_kill_ra(LLVMValueRef f) {
    LLVMValueRef i, loc;
    LLVMBasicBlockRef bb;
    std::set<LLVMValueRef> all_loads;
    std::set<LLVMValueRef>::iterator it;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> kill_ra;

    if (f == NULL) {
        std::cerr << "Invalid argument to function.\n";
        return std::nullopt;
    }

   // Create set of all loads instructions in program.
    for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
        for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
            if (LLVMGetInstructionOpcode(i) == LLVMLoad) all_loads.insert(i);
        }
    }

    for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
        // Create an empty kill set for this basic block.
        kill_ra.insert({bb, std::set<LLVMValueRef>()});

        for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
            if (LLVMGetInstructionOpcode(i) == LLVMStore) {
                // Get store location.
                loc = LLVMGetOperand(i, 1);

                // Look for load instructuons that are killed by this store.
                for (it = all_loads.begin(); it != all_loads.end(); ++it) 
                    // Add such loads to kill set.
                    if (LLVMGetOperand(*it, 0) == loc) kill_ra[bb].insert(*it);
            }
        }
    }

    return kill_ra;
}

/*
 * Computes IN and OUT set for all basic blocks of a function using reverse analysis.
 * IN and OUT sets take the form of a map from basic block references to sets of instructions.
 *
 * Args:
 * - f (LLVMValueRef): function for which to compute set.
 * - gen_ra: GEN set for correspinding function.
 * - kill_ra: KILL set for corresponding function.
 *
 * Returns:
 * - Pair of computed in and out sets, nullopt if failure.
 */
static std::optional<std::pair<std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>>> compute_in_and_out_ra(LLVMValueRef f, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>& gen_ra, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>& kill_ra) {
    LLVMValueRef term;
    LLVMBasicBlockRef bb, succ;
    std::set<LLVMBasicBlockRef>::iterator bb_it;
    std::set<LLVMValueRef>::iterator val_it;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMBasicBlockRef>> succs;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> old_in, in_ra, out_ra;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>::iterator map_it;
    bool change;
    unsigned int num;

    if (f == NULL) {
        std::cerr << "Invalid argument to function.\n";
        return std::nullopt;
    }

   // Compute successor sets for each basic block.
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

    // Initialize IN and OUT sets.
    for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
        // OUT sets start out as empty.
        out_ra.insert({bb, std::set<LLVMValueRef>()});

        // IN set is GEN set of corresponding basic block.
        in_ra.insert({bb, gen_ra.find(bb)->second});
    }

    // Iterate until a fixed point is reached.
    do {
        // First update OUT set.
        for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
            // OUT set is union of IN sets of all successors.
            for (bb_it = succs[bb].begin(); bb_it != succs[bb].end(); ++bb_it) {
                for (val_it = in_ra[*bb_it].begin(); val_it != in_ra[*bb_it].end(); ++val_it) {
                    out_ra[bb].insert(*val_it);
                }
            }
        }

        // Then update IN set.
        old_in = in_ra;
        for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
            // IN set contains GEN set of basic block.
            for (val_it = gen_ra[bb].begin(); val_it != gen_ra[bb].end(); ++val_it)
                    in_ra[bb].insert(*val_it);

            // IN set also contains union of set difference of OUT set and KILL set.
            for (val_it = out_ra[bb].begin(); val_it != out_ra[bb].end(); ++val_it)
                // Ensure not in KILL set.
                if (!kill_ra[bb].contains(*val_it))
                    in_ra[bb].insert(*val_it);
        }

        // Check whether IN set has changed.
        change = false;
        for (map_it = old_in.begin(); map_it != old_in.end() && !change; ++map_it) {
            // Check if old in and new in have the same number of statements.
            if (map_it->second.size() != in_ra[map_it->first].size()) change = true;
            for (val_it = map_it->second.begin(); val_it != map_it->second.end() && !change; ++val_it)
                // Ensure that all elements in old in are in new in.
                if (!in_ra[map_it->first].contains(*val_it)) change = true;
        }

    } while (change);

    return std::make_pair(in_ra, out_ra);
}


/*
 * Default constructor for Optimizer object.
 *
 * Returns:
 * - Optimizer: object with NULL-initialized LLVM module.
 */
Optimizer::Optimizer(void) {
    m = NULL;
}

/*
 * Constructs Optimizer object given LLVM file name.
 *
 * Args:
 * - fname (std::string&): LLVM file name
 *
 * Returns:
 * - Optimizer: object with fully initialized LLVM module
 *
 * Raises:
 * - runtime_error: module creation failed
 */
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

/*
 * Destructor for Optimizer object.
 */
Optimizer::~Optimizer(void) {
    if (m != NULL) LLVMDisposeModule(m);
}

/*
 * Writes LLVM module to file.
 * 
 * Args:
 * - fname (std::string&): LLVM File to which to write module.
 */
void Optimizer::write_to_file(std::string& fname) {
    char *err;

    if (LLVMPrintModuleToFile(m, fname.c_str(), &err) != 0) {
        std::cerr << err << std::endl;
        LLVMDisposeMessage(err);
    }
}

/*
 * Performs all optimizations on program
 * Optimizations include:
 * - Common subexpression elimination
 * - Dead code eliminiation
 * - Constant folding
 * - Constant propagation
 * - Live variable analysis.
 *
 * Optimizes until reaching a fixed point.
 *
 */
void Optimizer::optimize(void) {
    LLVMValueRef f;
    LLVMBasicBlockRef bb;
    bool sec, dce, cf, cp, changes, inner_changes, lva;



    // Walk through all basic blocks in each function.
    do {
        changes = false;
        for (f = LLVMGetFirstFunction(m); f != NULL; f = LLVMGetNextFunction(f)) {
            // Perform local optimizations.
            for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
                sec = common_sub_expr_elim(bb);
                dce = dead_code_elim(bb);
                if (sec || dce) changes = true;
            }

            do {
                inner_changes = false;
                // Perform constant propagation.
                cp = constant_propagation(f);

                if (cp) inner_changes = true;

                // Perform constant folding.
                for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
                    cf = constant_folding(bb);
                    if (cf) inner_changes = true;
                }
            } while (inner_changes);

            // Perform live variable analysis.
            lva = live_variable_analysis(LLVMGetFirstFunction(m));
            if (lva) changes = true;
        }
    } while (changes);


}

/*
 * Prints out all instructions in each basic block of all sets.
 */
void Optimizer::print_set(std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>& print_set) {
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>::iterator map_it;
    std::set<LLVMValueRef>::iterator set_it;

    for (map_it = print_set.begin(); map_it != print_set.end(); ++map_it) {
        std::cout << "Block " << map_it->first << ":\n";
        for (set_it = map_it->second.begin(); set_it != map_it->second.end(); ++set_it) {
            LLVMDumpValue(*set_it);
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}

/*
 * Performs constant propagation.
 * Replaces uses of constant load instructions with a constant.
 * Uses sets computed wiht forward analysis.
 *
 * Args:
 * - f (LLVMValueRef): function on which to perform optimizations
 *
 * Returns:
 * - True is changes to LLVM module, false otherwise
 */
bool Optimizer::constant_propagation(LLVMValueRef f) {
    LLVMValueRef i, operand;
    LLVMBasicBlockRef bb;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> r, gen_fa, kill_fa, in_fa, out_fa;
    std::optional<std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>> opt_map;
    std::optional<std::pair<std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>>> opt_pair;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>::iterator map_it;
    std::set<LLVMValueRef>::iterator set_it;
    bool changes, same_val;
    std::set<LLVMValueRef> deletions, stores, killed;
    int val, j;

    if (f == NULL) {
        std::cerr << "Invalid argument to function.\n";
        return false;
    }

    // Compute relevant sets.
    opt_map = compute_gen_fa(f);
    if (!opt_map.has_value()) return false;
    else gen_fa = opt_map.value();
    opt_map = compute_kill_fa(f);
    if (!opt_map.has_value()) return false;
    else kill_fa = opt_map.value();
    opt_pair = compute_in_and_out_fa(f, gen_fa, kill_fa);
    if (!opt_pair.has_value()) return false;
    else {
        in_fa = opt_pair.value().first;
        out_fa = opt_pair.value().second;
    }

    // Initialize R[B] = IN[B].
    for (map_it = in_fa.begin(); map_it != in_fa.end(); ++map_it)
        r.insert({map_it->first, map_it->second});

    changes = false;
    for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
        for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
            if (LLVMGetInstructionOpcode(i) == LLVMStore) {
                // Check to see which instructions are killed by i.
                killed = find_instrs_with_operand(r[bb], LLVMGetOperand(i, 0));

                // Remove all instructions from R.
                for (set_it = killed.begin(); set_it != killed.end(); ++set_it) r[bb].erase(*set_it);

                // Add store instruction to R.
                r[bb].insert(i);
            } else if (LLVMGetInstructionOpcode(i) == LLVMLoad) {
                // Find all stores that store to location of load instruction.
                stores = find_instrs_with_operand(r[bb], LLVMGetOperand(i, 0));

                // Check is all stores are to the same value.
                same_val = true;
                j = 0;
                for (set_it = stores.begin(); set_it != stores.end() && same_val; ++set_it) {
                    operand = LLVMGetOperand(*set_it, 0);
                    if (LLVMIsConstant(operand) && LLVMGetTypeKind(LLVMTypeOf(operand)) == LLVMIntegerTypeKind) {
                        if (j == 0) val = LLVMConstIntGetSExtValue(LLVMGetOperand(*set_it, 0));
                        else if (val != LLVMConstIntGetSExtValue(LLVMGetOperand(*set_it, 0))) same_val = false;
                        j++;
                    } else same_val = false;
                }

                // Replace all uses of load with constant value and mark load for deletion.
                if (same_val && !stores.empty()) {
                    LLVMReplaceAllUsesWith(i, LLVMConstInt(LLVMInt32Type() , val, true));
                    deletions.insert(i);
                    changes = true;
                }
            }
        }
    }

    // Delete all marked load instructions.
    for (set_it = deletions.begin(); set_it != deletions.end(); ++set_it)
        LLVMInstructionEraseFromParent(*set_it);

    return changes;
}

/*
 * Performs live variable analysis.
 * Uses sets computed with reverse analysis.
 * Deletes all redundant store instructions.
 *
 * Args:
 * - f (LLVMValueRef): function on which to perform optimizations
 *
 * Returns:
 * - True is changes to LLVM module, false otherwise
 */
bool Optimizer::live_variable_analysis(LLVMValueRef f) {
    LLVMValueRef i;
    LLVMBasicBlockRef bb;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>> r, gen_ra, kill_ra, in_ra, out_ra;
    std::optional<std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>> opt_map;
    std::optional<std::pair<std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>, std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>>> opt_pair;
    std::unordered_map<LLVMBasicBlockRef, std::set<LLVMValueRef>>::iterator map_it;
    std::set<LLVMValueRef>::iterator set_it;
    bool changes;
    std::set<LLVMValueRef> deletions, loads, in_out, in_block;

    if (f == NULL) {
        std::cerr << "Invalid argument to function.\n";
        return false;
    }

    // Compute relevant sets.
    opt_map = compute_gen_ra(f);
    if (!opt_map.has_value()) return false;
    else gen_ra = opt_map.value();
    opt_map = compute_kill_ra(f);
    if (!opt_map.has_value()) return false;
    else kill_ra = opt_map.value();
    opt_pair = compute_in_and_out_ra(f, gen_ra, kill_ra);
    if (!opt_pair.has_value()) return false;
    else {
        in_ra = opt_pair.value().first;
        out_ra = opt_pair.value().second;
    }

    changes = false;
    for (bb = LLVMGetFirstBasicBlock(f); bb != NULL; bb = LLVMGetNextBasicBlock(bb)) {
        // COmpute set of all loads for basic block.
        loads = std::set<LLVMValueRef>();
        for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i))
             if (LLVMGetInstructionOpcode(i) == LLVMLoad) loads.insert(i);

        for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
            // If a load instruction, remove from set of loads.
            if (LLVMGetInstructionOpcode(i) == LLVMLoad) loads.erase(i);
            else if (LLVMGetInstructionOpcode(i) == LLVMStore) {
                // Check if there are any loads from the same location in out set.
                in_out = find_instrs_with_operand(out_ra[bb], LLVMGetOperand(i, 1));

                // Check if there are any loads from the same location in the basic block after store instruction.
                in_block = find_instrs_with_operand(loads, LLVMGetOperand(i, 1));

                if (in_out.empty() && in_block.empty()) {
                    changes = true;
                    deletions.insert(i);
                }
            }
        }
    }

    // Delete all marked load instructions.
    for (set_it = deletions.begin(); set_it != deletions.end(); ++set_it)
        LLVMInstructionEraseFromParent(*set_it);

    return changes;
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
    LLVMValueRef i, j;
    LLVMOpcode op_i, op_j;
    bool changes, store_found, same;
    int k;

    // Ensure block exists.
    if (bb == NULL) {
        std::cerr << "Invalid argument to common subexpression elimination function.\n";
        return false;
    }

    changes = false;

    for (i = LLVMGetFirstInstruction(bb); i != NULL; i = LLVMGetNextInstruction(i)) {
        // Do not eliminate all kinds of instructions.
        if (LLVMGetInstructionOpcode(i) != LLVMCall && LLVMGetInstructionOpcode(i) != LLVMStore && !LLVMIsATerminatorInst(i) && LLVMGetInstructionOpcode(i) != LLVMAlloca) {
            store_found = false;
            for (j = LLVMGetNextInstruction(i); j != NULL && !store_found; j = LLVMGetNextInstruction(j)) {
                op_i = LLVMGetInstructionOpcode(i);
                op_j = LLVMGetInstructionOpcode(j);

                // There may be no store instruction to the same location as a load instruction.
                if (op_i == LLVMLoad && op_j == LLVMStore && LLVMGetOperand(i, 0) == LLVMGetOperand(j, 1)) store_found = true;
                else if (op_i == op_j && LLVMGetFirstUse(j) != NULL) {
                    // All operands should be the same.
                    same = true;
                    for (k = 0; k < LLVMGetNumOperands(i); k++)
                        if (LLVMGetOperand(i, k) != LLVMGetOperand(j, k))
                            same = false;

                    if (same) {
                        // Replace uses of j with i.
                        LLVMReplaceAllUsesWith(j, i);
                        changes = true;
                    }
                }
            }
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
        std::cerr << "Invalid argument to dead code elimination function.\n";
        return false;
    }

    changes = false;
    prev = NULL;

    for (i = LLVMGetFirstInstruction(bb); i != NULL; i = next) {
        // Instruction is deleted if not used, not a store, alloc or not a terminator.
        if (LLVMGetFirstUse(i) == NULL && LLVMGetInstructionOpcode(i) != LLVMStore && LLVMGetInstructionOpcode(i) != LLVMAlloca && !LLVMIsATerminatorInst(i)) {
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
    std::set<LLVMValueRef>::iterator set_it;
    std::set<LLVMValueRef> deletions;

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

                // Mark instruction to be deleted.
                deletions.insert(i);

                changes = true;
            }
        }
    }

    // Delete all marked instructions.
    for (set_it = deletions.begin(); set_it != deletions.end(); ++set_it)
        LLVMInstructionEraseFromParent(*set_it);

    return changes;
}
