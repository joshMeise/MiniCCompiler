LLVMEXECS=main
LLVMFILES=test.ll
LLVMFLAGS0=-S -emit-llvm
CLANG=clang
CXX=g++
CXXFLAGS=-Wall -Wpedantic -I../utils -std=c++20
LLVMFLAGS1=`llvm-config-17 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-17/
LIBS=-lutils
LDFLAGS=-L../lib

all: $(LLVMEXECS) $(LLVMFILES)

$(LLVMEXECS): %: %.cpp
	$(CXX) $(CXXFLAGS) $(WNO) $< $(LDFLAGS) $(LIBS) $(LLVMFLAGS1) -o $@

%.ll: %.c
	$(CLANG) $(LLVMFLAGS0) $< -o $@

clean:
	rm *.ll -rf $(LLVMEXECS)
