/*
 * Questions:
 * - for loops?
 * - else if?
 * - can identifiers start with underscores?
 */

%{

#include "y.tab.h"
extern int yyerror(char*);

%}

%%

/* Keywords */
int                             { return INT; }
void                            { return VOID; }
extern                          { return EXTERN; }
print                           { return PRINT; }
read                            { return READ; }
if                              { return IF; }
else                            { return ELSE; }
while                           { return WHILE; }
return                          { return RETURN; }

/* Arithmetic operators */
+                               { return PLUS; }
-                               { return MINUS; }
*                               { return TIMES; }
/                               { return DIVIDE; }
=                               { return EQUALS; }

/* Conditionals */
<                               { return LT; }
>                               { return GT; }
<=                              { return LEQ; }
>=                              { return GEQ; }
==                              { return EQ; }

/* Miscellaneous */
[0-9]+                          { return NUMBER; }
[_A-Za-z][A-Za-z0-9_]           { return IDENTIFIER; }
[ \n\r\t]                       /* ignore whitespace */
.                               { return yytext[0]; }

%%

int yywrap(void) {
    return 1;
}
