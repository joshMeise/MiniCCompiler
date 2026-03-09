extern void print(int);
extern int read();

int func(int p) {   // 2
	int a;          // 3
	int b;          // 4
	int c;          // 5
    int d;          // 6

    a = p;

    b = a + 10;

    if (a < b)
        b = a + 30;

    c = p * 10;

    d = a * 10;

    b = b + c;
    b = b + d;

    print(d);

    d = c + b;

    print(d);

    return b + p;
}
