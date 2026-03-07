extern void print(int);
extern int read(void);

int func(int p) {   // 2
	int a;          // 3
	int b;          // 4
	int c;          // 5
    int d;          // 6

    a = p;

    b = a + 10;

    c = p * 10;

    d = a * 10;

    return c + p;
}
