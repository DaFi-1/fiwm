# Look at setup():
# If we run xrandr before updategeom():
# const char *xr[] = { "/bin/sh", "-c", "[ -x $HOME/.config/i3/xrandr.sh ] && $HOME/.config/i3/xrandr.sh || (command -v xrandr >/dev/null 2>&1 && xrandr --output eDP-1 --mode 1366x768 --rate 60 --pos 0x0 --output HDMI-1 --mode 1920x1080 --rate 60 --pos 1366x0 --primary)", NULL };
# long pid = sys_fork();
# if (pid == 0) {
#     sys_execve("/bin/sh", (char *const *)xr, envp_global);
#     sys_exit(1);
# }
# if (pid > 0) sys_wait4(pid, NULL, 0, NULL);
#
# THEN call updategeom()!
# Now updategeom() will see 2 screens (eDP-1 + HDMI-1)!
# nunique will be 2!
# tagmap will NOT be overwritten!
# tagmap[8] remains 0 (Monitor 0)!
# tagmap[0..7] remains 1 (Monitor 1)!
#
# AND in focusworkspace():
# For hiding windows:
# for (Client *fc = clients; fc; fc = fc->next) {
#     if (fc->workspace == oldtag) {
#         int vis = 0;
#         for (Monitor *tm = mons; tm; tm = tm->next)
#             if (tm != m && tm->curtag == oldtag) { vis = 1; break; }
#         if (!vis) hide_window(dpy, fc->win);
#     }
# }
# Look how elegant and safe:
# If ANY monitor is still currently viewing oldtag (like Monitor 0 viewing Workspace 9),
# it NEVER hides the windows of that workspace!
