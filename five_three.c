int cond(int x)
{
	return (x%5 == 0) ? 1 : 0;
}

int cond2(int x)
{
	if(x%3 == 0)
		return 1;
	return 0;
}

int main(int argc, char**argv)
{
	int x;
	x = 0;
	printf("numbers that're divisble by 5 and 3\n");
	while(x < 100)
	{
		if(cond(x) && cond2(x))
			printf("%d\n", x);
		x++;
	}
	return 0;
}