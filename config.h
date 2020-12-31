#include "types.h"

#define CHARGING "Charging"
#define DISCHARGING "Discharging"
#define FULL "Full"

#define GETVOLUME "pulsemixer --get-volume"
#define GETMUTE "pulsemixer --get-mute"

#define UPDATEINTERVAL_MS 500

#define AUDIOBACKEND_PULSE	0
#define AUDIOBACKEND_ALSA	1

static const char *datetimefmt[] = {
	"%a %b %d %H:%M:%S", NULL
};

static const char * batteryargs[] = {
	"/sys/class/power_supply/BAT0/capacity", "/sys/class/power_supply/BAT0/status", NULL
};



struct module modules[] = {
	{ "date",	{ .v = datetimefmt },		getdatetime,	1 },
	{ "volume",	{ .i = AUDIOBACKEND_PULSE },		getvolume,	1 },
	{ "battery",	{ .v = batteryargs },		getbattery,	0 }
}