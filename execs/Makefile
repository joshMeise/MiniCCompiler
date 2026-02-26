CXX=g++
CXXFLAGS=-Wall -Wpedantic -I../utils -std=c++20
WNO=-Wno-deprecated
LDFLAGS=-L../lib
LIBS=-lutils
LLVMFLAGS=`llvm-config-17 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-17/
EXECS=compiler

all: $(EXECS)

$(EXECS): %: %.cpp
	$(CXX) $(CXXFLAGS) $(WNO) $< $(LDFLAGS) $(LIBS) $(LLVMFLAGS) -o $@

clean:
	rm -rf $(EXECS)
