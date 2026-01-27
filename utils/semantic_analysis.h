/*
 * semantic_analysis.h - header file for semantic analysis function
 *
 * Josh Meise
 * 01-25-2026
 * Description:
 * - Performs semantic analysis on a MiniC program.
 * - Ensures all variables are declared before they are used.
 * - Ensures that variables are only declared once within each scope.
 * - Takes the root node of an AST as input.
 * - Outputs 0 if program is semantically sound, -1 is the tree is invalid, number of semantic errors otherwise.
 *
 */

#pragma once
#include "ast.h"

/*
 * Performs semantic analysis in an AST.
 * Ensures that all variables are declared before they are used.
 * Ensures that variables are only declared once within a block.
 *
 * Arguments:
 *      - root (astNode*): pointer to the root node of the AST
 *
 * Returns:
 *      - int: 0 if semantically sound, 1 otherwise
 */
int semantically_analyze(astNode* root);
