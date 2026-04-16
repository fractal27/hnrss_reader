# HNRSS Reader

HackerNews rss reader with a formatting to help unix utilities interactiveness.
the output of `hnrss_reader` is the simplest you could imagine:
```
title|link
title|link
...
```
for example you could do(taken from my config.h(dwm):

```
ws "$(hnrss_reader -n 20 | dmenu | cut -d '|' -f2)"
# this takes the last 20 posts done and basically searches through the titles,
# and pops open a browser window with the site linked to the post itself.
```

`ws` is a small utility, it just checks if the string as a parameter given is empty,
and if not it initializes a browser process based on the `$BROWSER` env. variable.

- This is under development, in no circumstances should you consider this "production-ready", but
its a useful utility, now(when I have the time)I will implement the [TODOs](TODO.md)
