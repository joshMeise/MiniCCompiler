/*
 * semantic_analysis.cpp - perform semantic analysis on MiniC program's AST
 *
 * Josh Meise
 * 01-25-2026
 * Description:
 * Ensures that a MiniC program is emantically sound.
 * No use before declaration and mutiple declaration consitute sound semantics.
 *
 * Citations:
 * - Used Professor Kommineni's ast.cpp file as a guideline for structure
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
            // If call has a parameter, perform semantic analysis incase it is a variable.
            if (stmt->call.param != NULL) {
                if ((rc = analyze_node(stmt->call.param)) == -1) return -1;
                errors += rc;
            }
            break;
        }
        case ast_ret: {
            // Perform semantic analysis on return statement.
            if ((rc = analyze_node(stmt->ret.expr)) == -1) return -1;
            errors += rc;
            break;
        }
        case ast_block: {
            // Add declarations to table and do analysis for block.
            for (auto node : (*stmt->block.stmt_list)) {
                if ((rc = analyze_node(node)) == -1) return -1;
                errors += rc;
            }

            // Delete block's symbol table.
            symbol_tables.pop_back();
            break;
        }
        case ast_while: {
            // Perform semantic analysis on while condition as part of outer scope.
            if ((rc = analyze_node(stmt->whilen.cond)) == -1) return -1;
            errors += rc;

            // DO analysis on while loop body.
            if ((rc = analyze_node(stmt->whilen.body)) == -1) return -1;
            errors += rc;
            break;
        }
        case ast_if: {
            // Perform semantic analysis on if condition as part of outer scope.
            if ((rc = analyze_node(stmt->ifn.cond)) == -1) return -1;
            errors += rc;

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
            // Perform semantic analysis to ensure all variables have been declared.
            if ((rc = analyze_node(stmt->asgn.lhs)) == -1) return -1;
            errors += rc;
            if ((rc = analyze_node(stmt->asgn.rhs)) == -1) return -1;
            errors += rc;
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
    std::string var_name;

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
            // No effect on semantic analysis.
            break;
        }
        case ast_var: {
            var_name = std::string(node->var.name);

            // If variable does not exist in symbol table there is an error.
            if (!identifier_exists(var_name))
                errors += 1;
            break;
        }
        case ast_cnst: {
            // No effect on semantic analysis.
            break;
        }
        case ast_rexpr: {
            // Ensure that any variables used are declared.
            if ((rc = analyze_node(node->rexpr.lhs)) == -1) return -1;
            errors += rc;
            if ((rc = analyze_node(node->rexpr.rhs)) == -1) return -1;
            errors += rc;
            break;
        }
        case ast_bexpr: {
            // Ensure that any variables used are declared.
            if ((rc = analyze_node(node->bexpr.lhs)) == -1) return -1;
            errors += rc;
            if ((rc = analyze_node(node->bexpr.rhs)) == -1) return -1;
            errors += rc;
            break;
        }
        case ast_uexpr: {
            // Ensure that any variables used are declared.
            if ((rc = analyze_node(node->uexpr.expr)) == -1) return -1;
            errors += rc;
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
    int ret;
    
    ret = analyze_node(root);

    if (ret == -1) std::cerr << "Error in semantic analysis.\n";
    else if (ret > 0) std::cerr << ret << " semantic errors found.\n";
    
    return ret;
}
