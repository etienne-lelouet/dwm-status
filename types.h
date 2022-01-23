#include <stddef.h>

struct args
{
	int i;
	float f;
	const void *v;
};

typedef struct module module;

struct module
{
	char name[32];
	struct args args;
	int (*init_func)(struct args);
	ssize_t (*loop_func)(module *, char *);
	char active;
	size_t max_chars_written;
};

struct activemodule 
{
	struct module *moduleptr;
};