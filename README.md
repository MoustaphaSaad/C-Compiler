# My little C compiler

So i like compilers also i like Ken Thompson and Dennis Ritchie.

One day i stumbled upon bits and pieces of the original source code of unix and c and i was amazed and immediately after i decided to start this c compiler project.

Note: The main design of instruction set and VM is based on C4.

**You can compile the code using this command** 

```
gcc -m32 cc.c -o cc -ldl
```

The result is a c compiler with a VM that can run c code.

you can compile and run programs with this command

`./cc hello.c`

or if you want to see the instructions as the VM executes it you can use the `-d`

`./cc -d hello.c` 

Those are examples of the programs it can run.

```c
int main(int argc, char** argv)
{
	printf("Hello, World!\n");
	return 0;
}
```

This program prints the hello world string

```c
int main(int argc, char** argv)
{
	int x;
	x = 0;
	while(x < 10)
	{
		printf("%d\n", x++);
	}
	return 0;
}
```

This program lists the number from 0 to 9

```c
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
	printf("numbers that're divisible by 5 and 3\n");
	while(x < 100)
	{
		if(cond(x) && cond2(x))
			printf("%d\n", x);
		x++;
	}
	return 0;
}
```

This program lists the numbers that're divisible by 5 and 3

```c
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
```

This program lists the factorial of numbers from 1 to 10

```c
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
```

This program lists the fibonacci of numbers from 0 to n you can provide n as argument. the default value to n is 10

`./cc fibonacci.c 20`

```c
int* push_front(int* root, int value)
{
	int *new_element;
	new_element = malloc(2*sizeof(int));

	new_element[0] = value;
	new_element[1] = (int)root;
	printf("element value: %d\n", new_element[0]);
	printf("element next: %p\n", new_element[1]);

	return new_element;
}

int print_list(int* root)
{
	int* it, i;
	it = root;
	i = 0;
	while(it != 0)
	{
		printf("element #%d: %d\n", i, it[0]);
		it = (int*)it[1];
		i++;
	}
	return 0;
}

int main(int argc, char** argv)
{
	int size, i, *root;
	size = 10;
	root = 0;
	i = 1;

	while(i < size)
	{
		root = push_front(root, i++);
	}
	print_list(root);

	return 0;
}
```

This program creates a linked list of the numbers from 0 to 9 then prints it