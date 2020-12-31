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

#include "config.h"

struct args {
	int i;
	float f;
	const void* v;
};

struct module {
	char name[32];
	int (*ptr) (struct args);
	struct args args;
	char active;
};

static Display *dpy;


void setstatus(char *str);
int getdatetime(struct args args, char* buffer);
int getvolume(struct args args, char* buffer);
int getbattery(struct args args, char* buffer);

void setstatus(char *str) {
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

int getdatetime(struct args args, char* buffer) {
	time_t result;
	struct tm *resulttm;
	char* fmt = (char *)(args.v[0])                                                                                                   ;
	result = time(NULL);
	resulttm = localtime(&result);
	if(resulttm == NULL) {
		fprintf(stderr, "Error getting localtime.\n");
		return -1;
	}
	if(!strftime(buffer, 64, "%a %b %d %H:%M:%S", resulttm)) {
		fprintf(stderr, "strftime is 0.\n");
		return NULL;
	}

	return 0;
}

int getvolume(struct args args, char* buffer) {
	FILE* fd;
	char* buf;
	int left;
	int right;
	int mute;

	if ((buf = malloc(sizeof(char)*30)) == NULL) {
		perror("malloc getvolume");
		return NULL;
	}

	fd = popen(GETVOLUME, "r");
	if (fd == NULL) {
		perror("getvolume");
		return NULL;
	}
	fscanf(fd, "%d %d", &left, &right);
	fclose(fd);

	fd = popen(GETMUTE, "r");
	if (fd == NULL) {
		perror("getmute");
		return NULL;
	}
	fscanf(fd, "%d", &mute);
	fclose(fd);
	sprintf(buf, "l:%d r:%d m:%d", left, right, mute);
	return buf;
}

int getbattery(struct args args, char* buffer) {
	// https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-power
	FILE *fd;
	int capacity;
	char chargingstatus[30];
	char chargingstatusres;
	char* buf;

	if ((buf = malloc(sizeof(char)*30)) == NULL) {
		perror("malloc getbattery");
		return NULL;
	}

	fd = fopen(BATTERYCAPPATH, "r");
	if(fd == NULL) {
		perror("getbatterycapacity");
		return NULL;
	}
	fscanf(fd, "%d", &capacity);
	fclose(fd);

	fd = fopen(BATTERYSTATUSPATH, "r");
	if(fd == NULL) {
		perror("getbatterystatus");
		return NULL;
	}
	fscanf(fd, "%s", chargingstatus);
	fclose(fd);
	if (strcmp(chargingstatus, DISCHARGING) == 0) {
		chargingstatusres = '-';
	} else if (strcmp(chargingstatus, CHARGING) == 0) {
		chargingstatusres = '+';
	} else if (strcmp(chargingstatus, FULL) == 0) {
		chargingstatusres = '=';
	} else {
		chargingstatusres = '?';
	}
	sprintf(buf, "%d%%%c", capacity, chargingstatusres);
	return buf;
}

int main(int argc, char** argv) {
	char *status;
	char *datetime;
	// char *bat0;
	char* volume;
	struct timespec rem;
	struct timespec req = {
		.tv_sec = UPDATEINTERVAL_MS / 1000,
		.tv_nsec = (UPDATEINTERVAL_MS % 1000) * 1000000 
	};
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], DEBUGOPTION) == 0) {
			setvbuf(stderr, (char*)NULL, _IOLBF, 0);
			setvbuf(stdout, (char*)NULL, _IOLBF, 0);
			printf("starting in debug mode :\
				buffering stdin and stdout line by line"
			);
		}
	}

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "Cannot open display.\n");
		return 1;
	}

	if((status = malloc(200)) == NULL)
		exit(1);

	for (;;nanosleep(&req, &rem)) {
		datetime = getdatetime();
		// bat0 = getbattery();
		volume = getvolume();
		if (datetime != NULL && volume != NULL) {
			snprintf(status, 200,
				"| %s | %s |",
				datetime, volume
			);
			setstatus(status);
		}

		free(datetime);
		// free(bat0);
		free(volume);
	}
	fprintf(stdout, "FIN : %s\n", datetime);

	free(status);
	XCloseDisplay(dpy);

	return 0;
}

