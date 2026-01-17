CC=gcc
LEX=lex
YACC=yacc
CFLAGS=-Wall -Wpedantic
EXECS=syntax

all: $(EXECS)

$(EXECS): %: %.l %.y
	$(YACC) -d $*.y
	$(LEX) $*.l
	$(CC) $(CFLAGS) lex.yy.c y.tab.c -o $@

clean:
	rm -rf $(EXECS) lex.yy.c y.tab.c y.tab.h y.output

