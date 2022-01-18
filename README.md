# dwm-status
A simple configurable utility to set DMW's status bar

Build using `make`, install using `make install`.

The volume parts relies on the `pactl-volumectl` utilities, available [here](https://github.com/etienne-lelouet/pactl-volumectl)

TODO:
- write a proper documentation
- Keeps the name of the running sink in a variable instead of calling `pactl-getsink` -n every tick
- have a (nice) icon to represent the selected ink instead of the (ugly) first three letters of the sink description