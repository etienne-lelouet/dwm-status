#ifndef TYPES_H
#define TYPES_H

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
	int (*init_func)(module *);
	ssize_t (*loop_func)(module *, char *);
	char active;
	size_t max_chars_written;
	module* nextactive;
};
#endif