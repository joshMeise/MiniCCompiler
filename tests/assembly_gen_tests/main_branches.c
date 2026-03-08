#include <stdio.h>

int func(int);

int read(void) {
    int x;
    scanf("%d", &x); 
    return x;
}

void print(int x) {
 printf("%d\n", x);
}

int main(void) {
    int i = func(-4);
    printf("%d\n", i);
    if (i == 96)
        return 0;
    else
        return 1;
}
