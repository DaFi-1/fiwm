# Look at the code in fiwm:
# 1. On startup:
# updategeom() is called.
# BUT xrandr has NOT run yet!
# So Xorg only sees 1 screen: 1366x768 (or cloned)!
# Since nunique == 1:
# for (int t = 0; t < WORKSPACE_COUNT; t++) tagmap[t] = 0;
# tagmap gets OVERWRITTEN! All workspaces are mapped to monitor 0!
# 2. THEN spawn(&autostart_cmd) runs xrandr!
# xrandr configures eDP-1 (1366x768) + HDMI-1 (1920x1080).
# But fiwm DOES NOT KNOW xrandr ran!
# Because fiwm already initialized with nunique = 1 and tagmap = all 0!
#
# So when you are running fiwm:
# fiwm thinks there is only 1 monitor!
# Both screens are just one giant or cloned desktop to fiwm!
# When you switch workspace, since all workspaces belong to monitor 0,
# switching from WS 9 to WS 1..8 hides WS 9 windows!
# And when you switch to WS 9, it shows them on whatever screen is active!
