CXX=g++
CXXFLAGS=-Wall -Wpedantic -std=c++20 -I../utils
WNO=-Wno-deprecated
LDFLAGS=-L../lib
LIBS=-lutils
LLVMFLAGS=`llvm-config-17 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-17/
EXECS=test_syntax test_semantics

all: $(EXECS) test_optimizations

test_optimizations: %: %.cpp
	$(CXX) $(CXXFLAGS) $(WNO) $< $(LDFLAGS) $(LIBS) $(LLVMFLAGS) -o $@

$(EXECS): %: %.c
	$(CXX) $(CXXFLAGS) $(WNO) $< $(LDFLAGS) $(LIBS) -o $@

clean:
	rm -rf $(EXECS) test_optimizations *.ll
