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
    int i = func(4);
    printf("%d\n", i);
    return 0;
}
