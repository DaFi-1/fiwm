#!/usr/bin/env bash
# Let's check what tagmap has on display :0 by running a probe or inspecting fiwm!
# In fiwm:
# static int tagmap[WORKSPACE_COUNT] = {
#     1, 1, 1, 1, 1, 1, 1, 1, 0
# };
# Notice: tagmap has:
# Workspaces 1..8 (tag 0..7) -> Monitor 1 (eDP-1)
# Workspace 9 (tag 8) -> Monitor 0 (HDMI-1)
