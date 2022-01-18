struct args
{
	int i;
	float f;
	const void *v;
};

struct module
{
	char name[32];
	struct args args;
	int (*init_func)(struct args);
	int (*loop_func)(struct args, char *);
	char active;
};

struct activemodule 
{
	struct module *moduleptr;
	char *allocatedbuffer;
};