CXXFLAGS=-Wall -Wpedantic -std=c++20
OFILES=ast.o y.tab.o lex.yy.o semantic_analysis.o optimizer.o
CXX=g++
LEX=lex
YACC=yacc
LLVMFLAGS=-I /usr/include/llvm-c-17/
WNO=-Wno-deprecated -Wno-writable-strings -Wno-write-strings

all: ../lib/libutils.a

../lib/libutils.a: $(OFILES)
	ar cr ../lib/libutils.a $(OFILES)

y.tab.c y.tab.h: parser.y
	$(YACC) -d parser.y

lex.yy.c: lexer.l y.tab.h
	$(LEX) lexer.l

optimizer.o: %.o: %.cpp
	$(CXX) $(LLVMFLAGS) $(CXXFLAGS) -c $<

%.o: %.c
	$(CXX) $(WNO) $(CXXFLAGS) -c $<

%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -rf *.output *.o y.tab.c y.tab.h lex.yy.c ../lib/libutils.a
