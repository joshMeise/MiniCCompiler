CLANG=clang
LLVMFILEFLAGS=-S -emit-llvm
LLVMFILES=test_basic.ll

exec: main.c test.s
	clang -m32 test.s main.c -o test

llvm_files: $(LLVMFILES)

%.ll: %.c 
	$(CLANG) $(LLVMFILEFLAGS) $< -o $@

clean:
	rm -rf *.ll *.s test
