extern void print(int);
extern int read(void);

int func(int s) {
    int a;
    int b;
    int c;
    int d;

    a = s;
    b = 10;
    c = b;

    if (s < b) {
        d = c + b;
        b = d + c;
    }

    return b;

}
