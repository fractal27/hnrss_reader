# HNRSS Reader

HackerNews reader with a formatting to help unix utilities interactiveness
for example(taken from my config.h(dwm):

```
ws "$(hnrss_reader -n 20 | fzy | cut -d '|' -f2)"
```

`ws` is a small utility, it just checks if the string as a parameter given is empty,
and if not it initializes a browser process based on the `$BROWSER` env. variable.

