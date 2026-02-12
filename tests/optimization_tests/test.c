int func(int p) {
    int a;
    int b;
    int c;

    a = 3;
    b = p;
    
    c = a + b;
    b = a + b;

    return a + b;
}
