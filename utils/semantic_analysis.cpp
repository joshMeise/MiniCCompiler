/*
 * semantic_analysis.cpp - 
 *
 * Josh Meise
 * 01-25-2026
 * Description: 
 *
 */

#include "semantic_analysis.h"
#include <vector>
#include <unordered_set>
#include <string>
#include <iostream>

static int analyze_node(astNode* node);
static int analyze_stmt(astStmt* stmt) ;
static bool identifier_exists(std::string& id);

static std::vector<std::unordered_set<std::string>> symbol_tables;

/*
 * Checks whether a given identifier exists in the symbol tables.
 * First checks the symbol table with the narrowest scope.
 * Then checks symbol tables with broader scopes.
 *
 * Arguments:
 *      - id (std::string&): Identifier for which to search.
 *
 * Returns:
 *      - true if symbol found, false otherwise.
 */
static bool identifier_exists(std::string& id) {
    // Check each symbol table for the identiier starting at the narrowest scope.
    for (auto it = symbol_tables.rbegin(); it != symbol_tables.rend(); it ++) {
        for (const auto& symbol : *it) {
            if (id == symbol) return true;
        }
    }
    return false;
}

/*
 * Performs semantic analysis on a statement.
 * Creates and builds symbol tables for block statements.
 * Ensures that there are no double declarations within the same scope.
 * Checks that all variables have been declared before use.
 * 
 * Arguments:
 *      - stmt (astStmt*): Statement on which to operate.
 *
 * Returns:
 *      - -1 if error, number of semantic errors found in statement and statements decendant of statement.
 */
static int analyze_stmt(astStmt* stmt) {
    int errors, rc;

    // Ensure statement exists.
    if (stmt == NULL) return -1;
    
    errors = 0;

    switch(stmt->type){
        case ast_call: {
            // No effect on symbol table consturction.
            break;
        }
        case ast_ret: {
            // No effect on symbol table construction.
            break;
        }
        case ast_block: {
            // Add declarations to table and do analysis for block.
            for (auto node : (*stmt->block.stmt_list)) {
                if ((rc = analyze_node(node)) == -1) return -1;
                errors += rc;
            }

            for (string s : symbol_tables.back())
                std::cout << s << "\n";
            std::cout << "popping\n";

            // Delete block's symbol table.
            symbol_tables.pop_back();
            break;
        }
        case ast_while: {
            // While condition has no effect on symbol table construction.

            // DO analysis on while loop body.
            if ((rc = analyze_node(stmt->whilen.body)) == -1) return -1;
            errors += rc;
            break;
        }
        case ast_if: {
            // If condition has no effect on symbol table construction.

            // Do analysis on if statemetn body.
            if ((rc = analyze_node(stmt->ifn.if_body)) == -1) return -1;
            errors += rc;

            // If an else part exists, do analysis on body.
            if (stmt->ifn.else_body != NULL) {
                if ((rc = analyze_node(stmt->ifn.else_body)) == -1) return -1;
                errors += rc;
            }
            break;
        }
        case ast_asgn: {
            // No effect on symbol table construction.
            break;
        }
        case ast_decl: {
            // Check if declaration exists in symbol table.
            if (symbol_tables.back().contains(stmt->decl.name)) errors += 1;
            // Insert symbol declaration into symbol table.
            else symbol_tables.back().insert(stmt->decl.name);
            break;
        }
        default: {
            return -1;
        }
    }

    return errors;

}

/*
 * Recursively traverses AST to perform semantic analysis.
 * Checks what kind of node is being operated on.
 * 
 * Arguments:
 *      - node (astNode*): Node on which to operate.
 *
 * Returns:
 *      - -1 if error, number of semantic errors found in statements decendant of node.
 */
static int analyze_node(astNode* node) {
    int errors;
    int rc;

    if (node == NULL) return -1;

    errors = 0;

    switch(node->type) {
        case ast_prog: {
            // Analyze program's function.
            if ((rc = analyze_node(node->prog.func)) == -1) return -1;
            errors += rc;
            break;
        }
        case ast_func: {
            // Create symbol table for function block.
            symbol_tables.push_back(std::unordered_set<string>());

            // If function has a parameter, add it to symbol table.
            if (node->func.param != NULL)
                symbol_tables.back().insert(node->func.param->stmt.decl.name);

            // Construct symbol table for outer function block.
            // Jump straight to analyzing statement to avoid constructing another symbol table.
            if ((rc = analyze_stmt(&(node->func.body->stmt))) == -1) return -1;
            errors += rc;
            break;
        }
        case ast_stmt: {
            // If statement is a block statement, create symbol table for it.
            if (node->stmt.type == ast_block)
                symbol_tables.push_back(std::unordered_set<string>());

            // Fill out symbol table for statement.
            if ((rc = analyze_stmt(&(node->stmt))) == -1) return -1;
            errors += rc;
            break;
        }
        case ast_extern: {
            // No effect on symbol table construction.
            break;
        }
        case ast_var: {
            // No effect on symbol table construction.
            break;
        }
        case ast_cnst: {
            // No effect on symbol table construction.
            break;
        }
        case ast_rexpr: {
            // No effect on symbol table construction.
            break;
        }
        case ast_bexpr: {
            // No effect on symbol table construction.
            break;
        }
        case ast_uexpr: {
            // No effect on symbol table construction.
            break;
        }
        default: {
            return -1;
        }
    }
    return errors;
}

/*
 * Performs semantic analysis in an AST.
 * Ensures that all variables are declared before they are used.
 * Ensures that variables are only declared once within a block.
 *
 * Arguments:
 *      - root (astNode*): pointer to the root node of the AST
 *
 * Returns:
 *      - int: -1 if error, 0 if semantically sound, number of errors otherwise
 */
int semantically_analyze(astNode* root) {
    return analyze_node(root);
}
