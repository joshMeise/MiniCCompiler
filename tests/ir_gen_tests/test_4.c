extern void print(int);
extern int read();

int func(int n){
	int sum;
	int i;
	int a;
	i = 0;
	sum = 0;
    a = 1;
	
	if (n <= 0) return 0;
	else
		while (i < n){ 
			a = a * 4;
			sum = sum + a;
			i = i + 1;
		}
	
	return sum;
}
