/* made by etienne 2020-03-08.
**
** Compile with:
** gcc -Wall -pedantic -std=c99 -lX11 status.c
** TODO:
** Implement call to getbattery according to BATTERYCHECK value
** clean, GNU style, arg parsing
** network status (long term, nmtui works fine for now)
*/

#include <X11/Xlib.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "types.h"
#include "config.h"

static Display *dpy;
char *status;

void sigterm_handler(int sig) {
  free(status);
  XCloseDisplay(dpy);
  exit(0);
}

void setstatus(char *str) {
  XStoreName(dpy, DefaultRootWindow(dpy), str);
  XSync(dpy, False);
}

int init_pactl_volumectl(module *module) {
  FILE *sinkfile_fd = fopen(((char **)(module->args.v))[0], "w");
  if (sinkfile_fd == NULL) {
    perror("fopen sinkfile");
    return -1;
  }

  FILE *popen_fd = popen("pactl-getsink", "r");
  if (sinkfile_fd == NULL) {
    perror("fopen sinkfile");
    return -1;
  }

  int first_sink_id;
  int scanf_res = fscanf(popen_fd, "%d\n", &first_sink_id);

  if (scanf_res == EOF) {
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
  strlen = strlen + 1;
  char *str_value = malloc(sizeof(char) * strlen);
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
  free(str_value);
  return 0;
}

ssize_t getdatetime(module *module, char *buffer) {
  time_t result;
  struct tm *resulttm;
  char *fmt = ((char **)(module->args.v))[0];
  result = time(NULL);
  resulttm = localtime(&result);
  if (resulttm == NULL) {
    fprintf(stderr, "Error getting localtime.\n");
    return -1;
  }

  return strftime(buffer, MODULEBUFFERSIZE, fmt, resulttm);
}

ssize_t getvolume(module *module, char *buffer) {
  FILE *fd;
  int left;
  int right;
  int mute;

  fd = popen("pactl-getvolume", "r");
  if (fd == NULL) {
    perror("getvolume");
    return -1;
  }
  fscanf(fd, "%d %d", &left, &right);
  fclose(fd);

  fd = popen("pactl-getvolume m", "r");
  if (fd == NULL) {
    perror("getmute");
    return -1;
  }
  fscanf(fd, "%d", &mute);
  fclose(fd);

  fd = popen("pactl-getsink -n", "r");
  if (fd == NULL) {
    perror("getmute");
    return -1;
  }
  char wholestring[256];
  char truncstr[4];

  int sinkmatchlen;
  sinkmatchlen = fscanf(fd, "%s", wholestring);
  if (sinkmatchlen != 1) {
    printf("sinkmatchlen is not 1, is : %d\n", sinkmatchlen);
    strncpy(truncstr, "NaS", 4);
  } else {
      strncpy(truncstr, wholestring, 3);
  }
  
  fclose(fd);
  return sprintf(buffer, "%s: l:%d r:%d m:%d", truncstr, left, right, mute);
}

ssize_t getbattery(module *module, char *buffer) {
  // https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-power
  FILE *fd;
  int capacity;
  char chargingstatus[30];
  char chargingstatusres;

  char *battercycappath = ((char **)(module->args.v))[0];
  char *batterystatuspath = ((char **)(module->args.v))[1];

  fd = fopen(battercycappath, "r");
  if (fd == NULL) {
    perror("getbatterycapacity");
    return -1;
  }
  fscanf(fd, "%d", &capacity);
  fclose(fd);

  fd = fopen(batterystatuspath, "r");
  if (fd == NULL) {
    perror("getbatterystatus");
    return -1;
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
  return sprintf(buffer, "%d%%%c", capacity, chargingstatusres);
}

int main(int argc, char **argv) {
  size_t nmodules;
  struct timespec rem, req, startup_sleep;

  req = (struct timespec){.tv_sec = UPDATEINTERVAL_MS / 1000,
                          .tv_nsec = (UPDATEINTERVAL_MS % 1000) * 1000000};
  
  startup_sleep = (struct timespec){.tv_sec = 1,
                          .tv_nsec = 0};

  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], DEBUGOPTION) == 0) {
      setvbuf(stderr, (char *)NULL, _IOLBF, 0);
      setvbuf(stdout, (char *)NULL, _IOLBF, 0);
      fprintf(stderr, "starting in debug mode :\
 buffering stderr and stdout line by line\n");
    }

    if (strcmp(argv[i], VERSIONOPTION) == 0) {
      // printing version and exiting
      printf("dwmstatus-%s\n", VERSION);
      exit(0);
    }
  }

  char* display = getenv("DISPLAY");

  if (display == NULL) {
    display = XDISPLAY;
  }

  printf("Trying to connect to display %s\n", display);

  int try = 0;
  do {
    dpy = XOpenDisplay(display);
    if (dpy != NULL) {
      break;
    }
    fprintf(stderr, "Cannot open  display, %s, try = %d.\n", display, try);
    try++;
    nanosleep(&startup_sleep, &rem);
  } while (dpy == NULL);

  nmodules = sizeof(modules) / sizeof(struct module);

  size_t statussize = 0;

  module *head;
  module *prevactive = NULL;
  
  for (int i = 0; i < nmodules; i++) {
    if (modules[i].active == 1) {
      statussize = statussize + modules[i].max_chars_written + 2;
      fprintf(stderr, "module %s status_size %lu\n", modules[i].name,
              modules[i].max_chars_written);
      if (prevactive == NULL)
        head = &(modules[i]);
      else
        prevactive->nextactive = &(modules[i]);

      prevactive = &(modules[i]);
    }
  }

  fprintf(stderr, "statussize=%lu\n", statussize);
  fprintf(stderr, "running init functions\n");

  module *curactive;

  for (curactive = head; curactive != NULL; curactive = curactive->nextactive) {
    if (curactive->init_func == NULL)
      continue;
    fprintf(stderr, "running init function for module : %s\n", curactive->name);
    if (curactive->init_func(curactive) == -1) {
      fprintf(stderr, "error when executing function\n");
      exit(1);
    }
  }
  if ((status = malloc(statussize)) == NULL) {
    fprintf(stderr, "error mallocing statussize (%lu bytes)\n", statussize);
    exit(1);
  }
  size_t position;
  ssize_t written_bytes;
  for (;; nanosleep(&req, &rem)) {
    // status = " | \0" doesn't work
    // status = " | " status[3] = '\0' doesn't work either but I don't have the
    // time right now to see why
    position = 0;
    status[position] = '\0';
    for (curactive = head; curactive != NULL;
         curactive = curactive->nextactive) {
      written_bytes = curactive->loop_func(curactive, &(status[position]));
      if (written_bytes < 0) {
        fprintf(stderr,
                "Error when executing function associated with module %s, "
                "printing perror\n",
                curactive->name);
        perror("ici");
      } else {
        strcat(status, " | ");
        position = strlen(status);
      }
    }
    setstatus(status); /*  */
  }

  free(status);

  XCloseDisplay(dpy);

  return 0;
}
