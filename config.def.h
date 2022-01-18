#define DEBUGOPTION "-d"

#define CHARGING "Charging"
#define DISCHARGING "Discharging"
#define FULL "Full"

#define UPDATEINTERVAL_MS 250

#define AUDIOBACKEND_PULSE	0
#define AUDIOBACKEND_ALSA	1

#define MODULEBUFFERSIZE	256

static const char *datetimefmt[] = {
	"%a %b %d %H:%M", NULL
};

static const char *batteryargs[] = {
	"/sys/class/power_supply/BAT0/capacity\0", "/sys/class/power_supply/BAT0/status\0", NULL
};

static const char *volumeargs[] = {
	"/tmp/pactl-volumectl-sink", "pactl-getvolume m\0", NULL
};

// Pour plus tard : la commande qui permet de récupérer toutes les connexions actives non externes :
// nmcli | sed -nE '/\(externally\)/,/^$/d;s/^(\S*?):\s+?connected.*$/\1/p'

struct module modules[] = {
	{ .name = "date\0",		.args = { .v = datetimefmt },								.loop_func = getdatetime, .init_func = NULL,	.active = 1 },
	{ .name = "volume\0",	.args = { .i = AUDIOBACKEND_PULSE, .v = volumeargs },		.loop_func = getvolume,	.init_func = init_pactl_volumectl, .active = 1 },
	{ .name = "battery\0",	.args = { .v = batteryargs },								.loop_func = getbattery, .init_func = NULL,	.active = 1 }
};
