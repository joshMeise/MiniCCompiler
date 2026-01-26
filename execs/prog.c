extern void print(int);
extern int read(void);

int main(void) {
    int a;
    a = -1;

    if (a < 4) 
        return 1;
    else
        return 4;

    if (a > 4) {
        int i;

        i = 5;
    } else {
        int i;
        i = 5;
        print(i);
    }

    return 0;
}
