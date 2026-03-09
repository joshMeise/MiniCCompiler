CLANG=clang
EXECS=test_1 test_2 test_3 test_4 test_5

all: $(EXECS)

$(EXECS): %: %.ll main_%.c
	clang -m32 $^ -o $@

clean:
	rm -rf *.ll $(EXECS)
