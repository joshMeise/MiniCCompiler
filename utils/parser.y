/*
 * syntax.y - parser for a MiniC program
 *
 * Josh Meise
 * 01-16-2026
 * Description: 
 * - Defines syntax for a MiniC program.
 *
 * Assumptions:
 * - All variables are decleared at the top of functions.
 * - Can have unused relational expressions in functions.
 * - Can return relational expressions.
 * - Can have any nesting of parentheses.
 * - Can have standalone expressions inside of code body.
 * - Only functions that can be called are read() and print().
 * - else statements match with innermost if statement unless otehrwise specified with braces.
 * 
 * Citations:
 *
 * Questions:
 * - Can we declare and define variables on the same line?
 *      - Declarations and definitions cannot be combined.
 * - Can if statement and while loop conditions just be a single variable (i.e. not necessarily a full expression)?
 *      - Can be a single variable. Can just have a single term.
 * - Can we have void functions?
 *      - No void functions.
 * - Should we expect return to always be at the bottom of function?
 *      - No. Returns may be treated like any other statement.
 *
 * TODO:
 * - Possibly (void); maybe define rule that allows open or word "void"
 * - Check semi-olons at the edn of statements.
 *
 */

%{

#include <stdio.h>
#include "ast.h"

extern int yylex(void);
extern int linenum;

int yyerror(char*);
astNode* root;

%}

%union {
    int ival;
    char* sval;
    astNode* nval;
    std::vector<astNode*>* vval;
}

%token <ival> NUMBER NEG_NUMBER
%token <sval> IDENTIFIER
%token INT VOID EXTERN PRINT READ IF ELSE WHILE RETURN PLUS MINUS TIMES DIVIDE EQUALS LT GT LEQ GEQ EQ
%type <nval> relational_expr lt_expr gt_expr leq_expr geq_expr eq_expr expr_arg read_expr arithmetic_expr plus_expr minus_expr times_expr divide_expr assignment_stmt assignment_expr expr print_expr print_arg condition_body condition return return_body return_stmt while_stmt if_stmt else_part stmt stmt_or_expr variable_dec code_block argument_body argument function read print program read_arg no_args
%type <vval> stmts_and_exprs variable_decs code_block_body preamble
%type <sval> function_name

%start program

/* Match else statement with innermost if statement in dangling else case. */
%nonassoc NOELSE
%nonassoc ELSE

%%

program: preamble function                                          { $$ = createProg((*$1)[0], (*$1)[1], $2); delete $1; root = $$; }
       ;

preamble: print read                                                { $$ = new vector<astNode*>; $$->push_back($1); $$->push_back($2); }
        ;

print: EXTERN VOID PRINT '(' INT ')' ';'                            { $$ = createExtern("print"); }
     ;

read: EXTERN INT READ '(' no_args ')' ';'                           { $$ = createExtern("read"); }
    ;

function: INT function_name argument code_block                     { $$ = createFunc($2, $3, $4); free($2); }
        ;

function_name: IDENTIFIER                                           { $$ = $1; }
             ;

argument: '(' argument ')'                                          { $$ = $2; }
        | '(' argument_body ')'                                     { $$ = $2; }
        ;

argument_body: INT IDENTIFIER                                       { $$ = createDecl($2); free($2); }
             | no_args                                              { $$ = $1; }
             ;

code_block: '{' code_block '}'                                      { $$ = $2; }
          | '{' code_block_body '}'                                 { $$ = createBlock($2); }
          ;

code_block_body: variable_decs stmts_and_exprs                      { $$ = new vector<astNode*>; $$->insert($$->end(), $1->begin(), $1->end()); $$->insert($$->end(), $2->begin(), $2->end()); delete $1; delete $2; }
               ;

variable_decs: variable_dec variable_decs                           { $2->push_back($1); $$ = $2; }
             | /* empty */                                          { $$ = new vector<astNode*>; }
             ;

variable_dec: INT IDENTIFIER ';'                                    { $$ = createDecl($2); free($2); }
            ;

stmts_and_exprs: stmt_or_expr stmts_and_exprs                       { $2->push_back($1); $$ = $2; }
               | /* empty */                                        { $$ = new vector<astNode*>; }
               ;

stmt_or_expr: stmt                                                  { $$ = $1; }
            | expr                                                  { $$ = $1; }
            ;

stmt: assignment_stmt ';'                                           { $$ = $1; }
    | while_stmt                                                    { $$ = $1; }
    | if_stmt                                                       { $$ = $1; }
    | return_stmt ';'                                               { $$ = $1; }
    ;

while_stmt: WHILE condition code_block                              { $$ = createWhile($2, $3); }
          | WHILE condition stmt_or_expr                            { vector<astNode*>* vec = new vector<astNode*>; vec->push_back($3); $$ = createWhile($2, createBlock(vec)); }
          ;

if_stmt: IF condition code_block else_part                          { $$ = createIf($2, $3, $4); }
       | IF condition stmt_or_expr else_part                        { vector<astNode*>* vec = new vector<astNode*>; vec->push_back($3); $$ = createIf($2, createBlock(vec), $4); }
       ;

else_part: ELSE code_block                                          { $$ = $2; }
         | ELSE stmt_or_expr                                        { vector<astNode*>* vec = new vector<astNode*>; vec->push_back($2); $$ = createBlock(vec); }
         | /* empty */ %prec NOELSE                                 { $$ = NULL; }
         ;

return_stmt: RETURN return                                          { $$ = createRet($2); }
           ;

return: '(' return ')'                                              { $$ = $2; }
      | return_body                                                 { $$ = $1; }
      ;

return_body: arithmetic_expr                                        { $$ = $1; }
           | relational_expr                                        { $$ = $1; }
           | NUMBER                                                 { $$ = createCnst($1); }
           | NEG_NUMBER                                             { $$ = createCnst($1); }
           | IDENTIFIER                                             { $$ = createVar($1); free($1); }
           ;

condition: '(' condition ')'                                        { $$ = $2; }
         | '(' condition_body ')'                                   { $$ = $2; }
         ;

condition_body: relational_expr                                     { $$ = $1; }
              | arithmetic_expr                                     { $$ = $1; }
              | read_expr                                           { $$ = $1; }
              | IDENTIFIER                                          { $$ = createVar($1); free($1); }
              | NUMBER                                              { $$ = createCnst($1); }
              | NEG_NUMBER                                          { $$ = createCnst($1); }
              ;

expr: assignment_expr                                               { $$ = $1; }
    | read_expr ';'                                                 { $$ = $1; }
    | print_expr ';'                                                { $$ = $1; }
    ;

assignment_stmt: IDENTIFIER EQUALS assignment_expr                  { $$ = createAsgn(createVar($1), $3); free($1); }
               ;

assignment_expr: arithmetic_expr                                    { $$ = $1; }
               | relational_expr                                    { $$ = $1; }
               | read_expr                                          { $$ = $1; }
               | IDENTIFIER                                         { $$ = createVar($1); free($1); }
               | NUMBER                                             { $$ = createCnst($1); }
               | NEG_NUMBER                                         { $$ = createCnst($1); }
               ;

arithmetic_expr: plus_expr                                          { $$ = $1; }
               | minus_expr                                         { $$ = $1; }
               | times_expr                                         { $$ = $1; }
               | divide_expr                                        { $$ = $1; }
               ;

plus_expr: expr_arg PLUS expr_arg                                   { $$ = createBExpr($1, $3, add); }
         ;

minus_expr: expr_arg MINUS expr_arg                                 { $$ = createBExpr($1, $3, sub); }
          ;

times_expr: expr_arg TIMES expr_arg                                 { $$ = createBExpr($1, $3, mul); }
          ;

divide_expr: expr_arg DIVIDE expr_arg                               { $$ = createBExpr($1, $3, divide); }
           ;

relational_expr: lt_expr                                            { $$ = $1; }
               | gt_expr                                            { $$ = $1; }
               | leq_expr                                           { $$ = $1; }
               | geq_expr                                           { $$ = $1; }
               | eq_expr                                            { $$ = $1; }
               ;

lt_expr: expr_arg LT expr_arg                                       { $$ = createRExpr($1, $3, lt); }
       ;

gt_expr: expr_arg GT expr_arg                                       { $$ = createRExpr($1, $3, gt); }
       ;

leq_expr: expr_arg LEQ expr_arg                                     { $$ = createRExpr($1, $3, le); }
        ;

geq_expr: expr_arg GEQ expr_arg                                     { $$ = createRExpr($1, $3, ge); }
        ;

eq_expr: expr_arg EQ expr_arg                                       { $$ = createRExpr($1, $3, eq); }
       ;

expr_arg: read_expr                                                 { $$ = $1; }
        | IDENTIFIER                                                { $$ = createVar($1); free($1); }
        | NUMBER                                                    { $$ = createCnst($1); }
        | NEG_NUMBER                                                { $$ = createCnst($1); }
        ;

read_expr: READ read_arg                                            { $$ = createCall("read", $2); }
         ;

read_arg: '(' read_arg ')'                                          { $$ = $2; }
        | '(' /* empty */ ')'                                       { $$ = NULL; }
        ;

print_expr: PRINT print_arg                                         { $$ = createCall("print", $2); }
          ;

print_arg: '(' print_arg ')'                                        { $$ = $2; }
         | '(' assignment_expr ')'                                  { $$ = $2; }
         ;

no_args: VOID                                                       { $$ = NULL; }
       | /* empty */                                                { $$ = NULL; }
       ;

%%

int yyerror(char* s) {
    fprintf(stderr, "%s on line %d\n", s, linenum);
    return 1;
}
