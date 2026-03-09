CLANG=clang
LLVMFILEFLAGS=-S -emit-llvm
LLVMFILES=test_basic.ll test_cfold.ll test_common_subexpr.ll test_cprop_1.ll test_cprop_2.ll
EXECS=basic cfold common_subexpr cprop_1 cprop_2

all: $(EXECS)

$(EXECS): %: test_%.ll main_%.c
	clang $^ -o $@

llvm_files: $(LLVMFILES)

%.ll: %.c 
	$(CLANG) $(LLVMFILEFLAGS) $< -o $@

clean:
	rm -rf *.ll $(EXECS)
