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