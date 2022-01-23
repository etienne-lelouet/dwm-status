# dwm-status
A simple configurable utility to set DMW's status bar.

Has to be launched as a service, so it can be run AFTER `pipewire.service` starts, else the initialization function of the audio module WILL fail.

First you have to run `pipewire` as a service at user login ([source](https://wiki.gentoo.org/wiki/PipeWire#Starting_PipeWire_with_user_session)) :

`systemctl --user enable --now pipewire.service`


Build using `make`, install using `make install`.

And then enable with :
`systemctl --user enable dwmstatus.service`

The volume part relies on the `pactl-volumectl` utilities, available [here](https://github.com/etienne-lelouet/pactl-volumectl)

# TODO (by order of priorty):
- Keeps the name of the running sink in a variable instead of calling `pactl-getsink` -n every tick => how to detect that the sink has changed ? => maybe insted of querying `pactl-getvolume` regularly make pactl-volumectl write a string to a file when volume is changed, so that dwmstatus just has to read a file (since the goal of the whole `pactl-volumectl` modifications was to reduce the number of forks)
- find new information to display : 
  - current connection name (easy enough through parsing the output of `nmcli`)
  - new mail ?
  - mpd
- write a proper documentation
- have a (nice) icon to represent the selected ink instead of the (ugly) first three letters of the sink description