CXXFLAGS=-Wall -Wpedantic -std=c++20
OFILES=ast.o y.tab.o lex.yy.o
CXX=g++
LEX=lex
YACC=yacc
WNO=-Wno-deprecated -Wno-writable-strings

all: ../lib/libutils.a

../lib/libutils.a: $(OFILES)
	ar cr ../lib/libutils.a $(OFILES)

y.tab.c y.tab.h: parser.y
	$(YACC) -d parser.y

lex.yy.c: lexer.l y.tab.h
	$(LEX) lexer.l

%.o: %.c
	$(CXX) $(WNO) $(CXXFLAGS) -c $<

%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -rf *.o y.tab.c y.tab.h lex.yy.c ../lib/libutils.a
