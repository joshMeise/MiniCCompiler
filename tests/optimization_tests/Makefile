CLANG=clang
LLVMFILEFLAGS=-S -emit-llvm
LLVMFILES=test.ll

all: $(LLVMFILES)

%.ll: %.c 
	$(CLANG) $(LLVMFILEFLAGS) $< -o $@

clean:
	rm -rf *.ll
