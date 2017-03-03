int factorial(int x)
{
	int i, result;
	i=1;
	result = 1;
	while(i <= x)
	{
		result = result * i++;
	}
	return result;
}

int main(int argc, char** argv)
{
	int x;
	x = 1;
	while(x <= 10)
	{
		printf("factorial(%d) = %d\n", x, factorial(x));
		x++;
	}
	return 0;
}