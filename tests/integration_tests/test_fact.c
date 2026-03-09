extern void print(int);
extern int read(void);

int func(int a) {
    int m;
    int i;
    int b;

    m = 1;
    i = 0;

    while (i < a) {
        i = i + 1;
        m = m * i;
    }

    return m;
}
