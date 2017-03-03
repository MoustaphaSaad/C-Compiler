int fibonacci(int n)
{
	if(n == 0)
		return 0;
	else if(n == 1)
		return 1;
	else
		return (fibonacci(n-1) + fibonacci(n-2));
}

int parseString(char* str)
{
	char* str_it;
	int result;
	result = 0;
	str_it = str;
	while(*str_it)
	{
		result = result * 10;
		result = result + *str_it - 48;	//48 = '0'
		str_it++;
	}
	return result;
}

int main(int argc, char** argv)
{
	int x,n;
	printf("argc = %d\n", argc);
	printf("argv = %s\n", *argv);
	argc--;
	argv++;
	if(argc == 0)
		n = 10;
	else
		n = parseString(*argv);

	while(x <= n)
	{
		printf("fibonacci(%d) = %d\n", x, fibonacci(x));
		x++;
	}
	return 0;
}