CC=g++
LEX=lex
YACC=yacc
CFLAGS=-Wall -Wpedantic -I../utils
WNO=-Wno-deprecated -Wno-c++11-extensions -Wno-c++11-compat-deprecated-writable-strings
LDFLAGS=-L../lib
LIBS=-lutils
EXECS=test_syntax

all: y.tab.o lex.yy.o

$(EXECS): %: %.c y.tab.o lex.yy.o
	     $(CC) $(CFLAGS) $(WNO) $^ $(LDFLAGS) $(LIBS) -o $@

y.tab.o: parser.y
		 $(YACC) -d $<
		 $(CC) $(CFLAGS) $(WNO) -c y.tab.c -o $@

lex.yy.o: lexer.l
		   $(LEX) $<
		   $(CC) $(CFLAGS) $(WNO) -c lex.yy.c -o $@

clean:
	rm -rf $(EXECS) *.o lex.yy.c y.tab.c y.tab.h y.output 

