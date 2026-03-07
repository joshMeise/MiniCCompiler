#include <stdio.h>

int func(void);

int read(void) {
    int x;
    scanf("%d", &x); 
    return x;
}

void print(int x) {
 printf("%d\n", x);
}

int main(void) {
    int i = func();
    printf("In main printing return value of test: %d\n", i);
    return 0;
}
