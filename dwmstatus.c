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
int getvolume(struct args args, char *buffer);
int getbattery(struct args args, char *buffer);

#include "config.h"

static Display *dpy;

void setstatus(char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
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

	char *volumeget = ((char **)(args.v))[0];
	char *muteget = ((char **)(args.v))[1];

	fd = popen(volumeget, "r");
	if (fd == NULL)
	{
		perror("getvolume");
		return -1;
	}
	fscanf(fd, "%d %d", &left, &right);
	fclose(fd);

	fd = popen(muteget, "r");
	if (fd == NULL)
	{
		perror("getmute");
		return -1;
	}
	fscanf(fd, "%d", &mute);
	fclose(fd);
	sprintf(buffer, "l:%d r:%d m:%d", left, right, mute);
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

	printf("statussize=%lu\n", statussize);

	if ((status = malloc(statussize)) == NULL)
		exit(1);

	for (;; nanosleep(&req, &rem))
	{	
		// status = " | \0" doesn't work
		// status = " | " status[3] = '\0' doesn't work either but I don't have the time right now to see why
		status[0] = '\0';
		for (curractivemoduleindex = 0; curractivemoduleindex < nmodulesactive; curractivemoduleindex++)
		{
			curractivemodule = activemodules[curractivemoduleindex];
			if (curractivemodule.moduleptr->ptr(curractivemodule.moduleptr->args, curractivemodule.allocatedbuffer) != 0)
			{
				fprintf(stderr, "Error when executing function associated with module %s\n", curractivemodule.moduleptr->name);
			}
			else
			{
				// printf("%s \n", curractivemodule.allocatedbuffer);

				// for (i = 0; i < statussize; i++)
				// {
				// 	if (status[i] == '\0') {
				// 		break;
				// 	}
				// 	printf("%c\n", status[i]);
				// }

				// for (i = 0; i < MODULEBUFFERSIZE; i++)
				// {
				// 	if (curractivemodule.allocatedbuffer[i] == '\0') {
				// 		break;
				// 	}
				// 	printf("%c\n", curractivemodule.allocatedbuffer[i]);
				// }
				if ((strlen(status) + strlen(curractivemodule.allocatedbuffer) + 4) <= statussize)
				{
					strcat(status, curractivemodule.allocatedbuffer);
					strcat(status, " | ");
				}
				else
				{
					fprintf(stdout, "Not enough space in status for strcat\n");
				}
				// snprintf(status, (nmodulesactive * MODULEBUFFERSIZE) * sizeof(char), "%s | %s", status, curractivemodule.allocatedbuffer);
			}
		}
		setstatus(status);
	}

	free(status);
	XCloseDisplay(dpy);

	return 0;
}
