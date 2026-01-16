/*
 * syntax.y - parser for a MiniC program
 *
 * Josh Meise
 * 01-16-2026
 * Description: 
 * - Defines syntax for a MiniC program
 *
 * Citations:
 * - ChatGPT for help getting rid of compiler warnings for unused functions.
 *
 * Questions:
 *
 * TODO:
 *
 */

%option noinput
%option nounput

%{

#include "y.tab.h"
extern int yyerror(char*);

%}

%%

"int"                           { return INT; }
"void"                          { return VOID; }
"extern"                        { return EXTERN; }
"print"                         { return PRINT; }
"read"                          { return READ; }
"if"                            { return IF; }
"else"                          { return ELSE; }
"while"                         { return WHILE; }
"return"                        { return RETURN; }
"+"                             { return PLUS; }
"-"                             { return MINUS; }
"*"                             { return TIMES; }
"/"                             { return DIVIDE; }
"="                             { return EQUALS; }
"<"                             { return LT; }
">"                             { return GT; }
"<="                            { return LEQ; }
">="                            { return GEQ; }
"=="                            { return EQ; }
[0-9]+                          { return NUMBER; }
[_A-Za-z][A-Za-z0-9_]*          { return IDENTIFIER; }
[ \n\r\t]                       
.                               { return yytext[0]; }

%%

int yywrap(void) {
    return 1;
}
