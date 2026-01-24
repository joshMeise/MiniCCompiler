/*
 * lexer.l - lexer for MiniC program
 *
 * Josh Meise
 * 01-16-2026
 * Description: 
 * - Gets tokens for MiniC program and sends them to parser.
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

int linenum = 1;

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
[0-9]+                          { yylval.ival = atoi(yytext); return NUMBER; }
[_A-Za-z][A-Za-z0-9_]*          { yylval.sval = strdup(yytext); return IDENTIFIER; }
[ \r\t]
[\n]                            { linenum++; }
.                               { return yytext[0]; }

%%

int yywrap(void) {
    return 1;
}
