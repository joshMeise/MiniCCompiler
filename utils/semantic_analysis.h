/*
 * semantic_analysis.h - 
 *
 * Josh Meise
 * 01-25-2026
 * Description: 
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
