CXX=g++
CXXFLAGS=-Wall -Wpedantic -std=c++20 -I../utils
WNO=-Wno-deprecated
LDFLAGS=-L../lib
LIBS=-lutils
LLVMFLAGS=`llvm-config-17 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-17/
CEXECS=test_syntax test_semantics
CXXEXECS=test_optimizations test_ir_gen

all: $(CEXECS) $(CXXEXECS)

$(CXXEXECS): %: %.cpp
	$(CXX) $(CXXFLAGS) $(WNO) $< $(LDFLAGS) $(LIBS) $(LLVMFLAGS) -o $@

$(CEXECS): %: %.c
	$(CXX) $(CXXFLAGS) $(WNO) $< $(LDFLAGS) $(LIBS) -o $@

clean:
	rm -rf $(CEXECS) $(CXXEXECS) *.ll
