extern void print(int);
extern int read();

int func(void){
	int max;
	int i;
	int a;
    int n;
	i = 0;
	max = 0;

    n = 5;
	
	while (i < n){ 
		a = i;
		if (a > max)
			max = a;
		i = i + 1;
	}
	
	return max;
}
