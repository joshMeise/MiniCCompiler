CXX=g++
CXXFLAGS=-Wall -Wpedantic -std=c++20 -I../utils
WNO=-Wno-deprecated
LDFLAGS=-L../lib
LIBS=-lutils
EXECS=test_syntax test_semantics

all: $(EXECS)

$(EXECS): %: %.c
	$(CXX) $(CXXFLAGS) $(WNO) $< $(LDFLAGS) $(LIBS) -o $@

clean:
	rm -rf $(EXECS)
