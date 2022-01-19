# dwm-status
A simple configurable utility to set DMW's status bar.

Has to be launched as a service, so it can be run AFTER `pipewire.service` starts, else the initialization function of the audio module WILL fail.

First you have to run `pipewire` as a service at user login ([source](https://wiki.gentoo.org/wiki/PipeWire#Starting_PipeWire_with_user_session)) :

`systemctl --user enable --now pipewire.service`


Build using `make`, install using `make install`.

And then enable with :
`systemctl --user enable dwmstatus.service`

The volume part relies on the `pactl-volumectl` utilities, available [here](https://github.com/etienne-lelouet/pactl-volumectl)

# TODO:
- write a proper documentation
- Keeps the name of the running sink in a variable instead of calling `pactl-getsink` -n every tick
- have a (nice) icon to represent the selected ink instead of the (ugly) first three letters of the sink description