#include <stdio.h>

int func(void);

int read(void) {
    int x;
    scanf("%d", &x); 
    return x;
}

void print(int x){
    printf("%d\n", x);
}

int main(void) {
    int i = func();
    printf("%d\n", i);
    if (i == 4)
        return 0;
    else
        return 1;
}
