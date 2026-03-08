CLANG=clang
LLVMFILEFLAGS=-S -emit-llvm
LLVMFILES=test_basic.ll test_branches.ll test_fact.ll test_fib.ll test_rem_2.ll test_square.ll
EXECS=basic branches fact fib rem_2 square

all: $(EXECS)

$(EXECS): %: test_%.s main_%.c
	clang -m32 $^ -o $@

llvm_files: $(LLVMFILES)

%.ll: %.c 
	$(CLANG) $(LLVMFILEFLAGS) $< -o $@

clean:
	rm -rf *.ll *.s $(EXECS)
