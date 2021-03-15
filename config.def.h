#define DEBUGOPTION "-d"

#define CHARGING "Charging"
#define DISCHARGING "Discharging"
#define FULL "Full"

#define UPDATEINTERVAL_MS 250

#define AUDIOBACKEND_PULSE	0
#define AUDIOBACKEND_ALSA	1

#define MODULEBUFFERSIZE	256

static const char *datetimefmt[] = {
	"%a %b %d %H:%M:%S", NULL
};

static const char *batteryargs[] = {
	"/sys/class/power_supply/BAT0/capacity\0", "/sys/class/power_supply/BAT0/status\0", NULL
};

static const char *volumeargs[] = {
	"pactl-getvolume\0", "pactl-getvolume m\0", NULL
};

struct module modules[] = {
	{ .name = "date\0",	.args = { .v = datetimefmt },					.ptr = getdatetime,	.active = 1 },
	{ .name = "volume\0",	.args = { .i = AUDIOBACKEND_PULSE, .v = volumeargs },		.ptr = getvolume,	.active = 1 },
	{ .name = "battery\0",	.args = { .v = batteryargs },					.ptr = getbattery,	.active = 1 }
};
