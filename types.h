struct args {
	short nargs;
	void* args;
};

struct module {
	char name[32];
	int (*ptr) (struct args);
	struct args args;
};