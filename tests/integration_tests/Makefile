CLANG=clang
EXECS=basic fact fib

all: $(EXECS)

$(EXECS): %: test_%.s main_%.c
	clang -m32 $^ -o $@

clean:
	rm -rf *.s $(EXECS)
