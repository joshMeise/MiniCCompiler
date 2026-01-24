CXX=g++
CXXFLAGS=-Wall -Wpedantic -I../utils
WNO=-Wno-deprecated
LDFLAGS=-L../lib
LIBS=-lutils
EXECS=test_syntax

all: $(EXECS)

$(EXECS): %: %.c
	$(CXX) $(CXXFLAGS) $(WNO) $< $(LDFLAGS) $(LIBS) -o $@

clean:
	rm -rf $(EXECS)
