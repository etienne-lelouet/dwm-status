/* made by etienne 2020-03-08.
**
** Compile with:
** gcc -Wall -pedantic -std=c99 -lX11 status.c
** TODO:
** Implement call to getbattery according to BATTERYCHECK value
** clean, GNU style, arg parsing
** network status (long term, nmtui works fine for now)
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <X11/Xlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include "types.h"

void setstatus(char *str);
int getdatetime(struct args args, char *buffer);
int init_pactl_volumectl(struct args args);
int getvolume(struct args args, char *buffer);
int getbattery(struct args args, char *buffer);

#include "config.h"

static Display *dpy;

void setstatus(char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

int init_pactl_volumectl(struct args args) {
	FILE* sinkfile_fd = fopen(((char **)(args.v))[0], "w");
	if (sinkfile_fd == NULL) {
		perror("fopen sinkfile");
		return -1;
	}

	FILE* popen_fd = popen("pactl-getsink", "r");
	if (sinkfile_fd == NULL) {
		perror("fopen sinkfile");
		return -1;
	}

	int first_sink_id;
	int scanf_res = fscanf(popen_fd, "%d\n", &first_sink_id); 

	if (scanf_res== EOF) {
		perror("pactl-getsink fscanf");
		return -1;
	}
	if (scanf_res < 0) {
		fprintf(stderr, "error scanning getsink res.\n");
		return -1;
	}
	int strlen = snprintf(NULL, 0, "%d", first_sink_id);
	if (strlen <= 0) {
		fprintf(stderr, "error when calculating output string len\n");
		return -1;
	}
	strlen = strlen +1;
	char *str_value = malloc(sizeof(char)*strlen);
	if (str_value == NULL) {
		fprintf(stderr, "error when mallocing memory for output string\n");
		return -1;
	}
	if (sprintf(str_value, "%d", first_sink_id) < 0) {
		fprintf(stderr, "error when outputing str representation of sink\n");
		return -1;
	};
	if (fputs(str_value, sinkfile_fd) <= 0) {
		fprintf(stderr, "error when writing sink id to file\n");
		return -1;
		
	}
	if (fsync(fileno(sinkfile_fd)) < 0) {
		perror("fsync");
		return -1;
	}
	fclose(sinkfile_fd);
	fprintf(stderr, "successfully wrote sink id, is %s\n", str_value);
	return 0;
}

int getdatetime(struct args args, char *buffer)
{
	time_t result;
	struct tm *resulttm;
	char *fmt = ((char **)(args.v))[0];
	result = time(NULL);
	resulttm = localtime(&result);
	if (resulttm == NULL)
	{
		fprintf(stderr, "Error getting localtime.\n");
		return -1;
	}
	if (!strftime(buffer, MODULEBUFFERSIZE, fmt, resulttm))
	{
		fprintf(stderr, "strftime is 0.\n");
		return -1;
	}

	return 0;
}

int getvolume(struct args args, char *buffer)
{
	FILE *fd;
	int left;
	int right;
	int mute;

	fd = popen("pactl-getvolume", "r");
	if (fd == NULL)
	{
		perror("getvolume");
		return -1;
	}
	fscanf(fd, "%d %d", &left, &right);
	fclose(fd);

	fd = popen("pactl-getvolume m", "r");
	if (fd == NULL)
	{
		perror("getmute");
		return -1;
	}
	fscanf(fd, "%d", &mute);
	fclose(fd);

	fd = popen("pactl-getsink -n", "r");
	if (fd == NULL)
	{
		perror("getmute");
		return -1;
	}
	char wholestring[256];
	char truncstr[4];
	fscanf(fd, "%s", wholestring);
	strncpy(truncstr, wholestring, 3);
	fclose(fd);
	sprintf(buffer, "%s : l:%d r:%d m:%d", truncstr, left, right, mute);
	return 0;
}

int getbattery(struct args args, char *buffer)
{
	// https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-power
	FILE *fd;
	int capacity;
	char chargingstatus[30];
	char chargingstatusres;

	char *battercycappath = ((char **)(args.v))[0];
	char *batterystatuspath = ((char **)(args.v))[1];

	fd = fopen(battercycappath, "r");
	if (fd == NULL)
	{
		perror("getbatterycapacity");
		return -1;
	}
	fscanf(fd, "%d", &capacity);
	fclose(fd);

	fd = fopen(batterystatuspath, "r");
	if (fd == NULL)
	{
		perror("getbatterystatus");
		return -1;
	}
	fscanf(fd, "%s", chargingstatus);
	fclose(fd);
	if (strcmp(chargingstatus, DISCHARGING) == 0)
	{
		chargingstatusres = '-';
	}
	else if (strcmp(chargingstatus, CHARGING) == 0)
	{
		chargingstatusres = '+';
	}
	else if (strcmp(chargingstatus, FULL) == 0)
	{
		chargingstatusres = '=';
	}
	else
	{
		chargingstatusres = '?';
	}
	sprintf(buffer, "%d%%%c", capacity, chargingstatusres);
	return 0;
}

int main(int argc, char **argv)
{
	char *status;
	int i, curractivemoduleindex;
	struct activemodule *activemodules;
	struct activemodule curractivemodule;
	char *currbuffer;
	size_t nmodules, nmodulesactive;
	struct timespec rem, req;

	req = (struct timespec){
	    .tv_sec = UPDATEINTERVAL_MS / 1000,
	    .tv_nsec = (UPDATEINTERVAL_MS % 1000) * 1000000};
	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], DEBUGOPTION) == 0)
		{
			setvbuf(stderr, (char *)NULL, _IOLBF, 0);
			setvbuf(stdout, (char *)NULL, _IOLBF, 0);
			printf("starting in debug mode :\
				buffering stdin and stdout line by line");
		}
	}

	if (!(dpy = XOpenDisplay(NULL)))
	{
		fprintf(stderr, "Cannot open display.\n");
		return 1;
	}

	nmodules = sizeof(modules) / sizeof(struct module);

	nmodulesactive = 0;

	for (i = 0; i < nmodules; i++)
	{
		if (modules[i].active == 1)
		{
			nmodulesactive++;
		}
	}

	if (nmodulesactive == 0)
	{
		fprintf(stderr, "no modules active");
		return 1;
	}

	activemodules = (struct activemodule *)malloc(nmodulesactive * sizeof(struct activemodule));

	curractivemoduleindex = 0;
	for (i = 0; i < nmodules; i++)
	{
		if (modules[i].active == 1)
		{
			currbuffer = (char *)malloc(MODULEBUFFERSIZE * sizeof(char));
			curractivemodule = (struct activemodule){
			    .moduleptr = &modules[i],
			    .allocatedbuffer = currbuffer};
			activemodules[curractivemoduleindex] = curractivemodule;
			curractivemoduleindex++;
		}
	}

	size_t statussize = nmodulesactive * MODULEBUFFERSIZE * sizeof(char);

	fprintf(stderr, "statussize=%lu\n", statussize);
	fprintf(stderr, "running init functions\n");
	for (curractivemoduleindex = 0; curractivemoduleindex < nmodulesactive; curractivemoduleindex++)
	{
		curractivemodule = activemodules[curractivemoduleindex];
		if (curractivemodule.moduleptr->init_func == NULL)
			continue;
		fprintf(stderr, "running init function for module : %s\n", curractivemodule.moduleptr->name);
		if (curractivemodule.moduleptr->init_func(curractivemodule.moduleptr->args) == -1) {
			fprintf(stderr, "error when executing function\n");
			exit(1);
		}
	}
	if ((status = malloc(statussize)) == NULL)  {
		fprintf(stderr, "error mallocing statussize (%lu bytes)\n", statussize);
		exit(1);
	}

	for (;; nanosleep(&req, &rem))
	{
		// status = " | \0" doesn't work
		// status = " | " status[3] = '\0' doesn't work either but I don't have the time right now to see why
		status[0] = '\0';
		for (curractivemoduleindex = 0; curractivemoduleindex < nmodulesactive; curractivemoduleindex++)
		{
			curractivemodule = activemodules[curractivemoduleindex];
			if (curractivemodule.moduleptr->loop_func(curractivemodule.moduleptr->args, curractivemodule.allocatedbuffer) != 0)
			{
				fprintf(stderr, "Error when executing function associated with module %s\n", curractivemodule.moduleptr->name);
			}
			else
			{
				if ((strlen(status) + strlen(curractivemodule.allocatedbuffer) + 4) <= statussize)
				{
					strcat(status, curractivemodule.allocatedbuffer);
					strcat(status, " | ");
				}
				else
				{
					fprintf(stdout, "Not enough space in status for strcat\n");
				}
			}
		}
		setstatus(status);
	}

	for (curractivemoduleindex = 0; curractivemoduleindex < nmodulesactive; curractivemoduleindex++)
	{
		curractivemodule = activemodules[curractivemoduleindex];
		free(curractivemodule.allocatedbuffer);
	}
	free(activemodules);

	free(status);
	XCloseDisplay(dpy);

	return 0;
}
