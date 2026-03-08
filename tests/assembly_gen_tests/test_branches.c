extern int read();
extern void print(int);

int func(int i){
	int a;
	int b;

	if (i > 100) {
        print(i);
		a = 10;
		b = 20;
	}
	else {
        print(i - 1);
		a = 100;
		b = i + 200;
	}
	
	return (i + a);
}

