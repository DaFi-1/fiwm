#!/usr/bin/env bash
# Let's test what happens when Xorg starts before xrandr:
# In fiwm.c setup():
# updategeom() is called BEFORE autostart_cmd (which calls xrandr)!
# Look at fiwm.c:
# Line 2174: updategeom();
# Line 2175: createbars();
# Line 2176: grabkeys();
# Line 2178: selmon = mons;
# Line 2179: for (Monitor *m = mons; m; m = m->next) arrange(m);
# Line 2182: Arg a = { .v = autostart_cmd };
# Line 2183: spawn(&a);
