extern int read();
extern void print(int);

int func(int p) {
    int a;
    int b;
    int c;

    read();
    print(a);

    b = p;
    
    c = a + b;
    b = a + b;

    return a + b;
}
