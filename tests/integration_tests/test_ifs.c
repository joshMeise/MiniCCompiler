extern void print(int);
extern int read(void);

int func(int p) {
    int a;
    int b;
    int c;
    int d;

    a = 4 * p;
    b = 4 * p;
    
    p = 13;
    
    c = 4 * p;

    d = 5 * p;

    if (c < d)
        return c;
    else
        return -32;

}
