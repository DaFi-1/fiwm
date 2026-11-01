# Let's check how windows are attached to workspaces:
# When a window is moved to workspace 9:
# movecmd:
# raw = 8 (Workspace 9)
# Client *c = n->u.client;
# Node *newfocus = node_detach(n, ws);
# c->workspace = 8;
# tree_insert(&workspaces[8], c, SPLIT_VERTICAL);
# Monitor *target_m = mon_from_tag(8);
# if (target_m) arrange(target_m);
# arrange(selmon);
#
# Now what is in &workspaces[8]?
# workspaces[8].root points to the node tree for Workspace 9!
#
# Now, what happens when user is on Monitor 1 (where curtag is 0, Workspace 1):
# User switches workspace: focusworkspace(&{.i = 1}) (Workspace 2).
# Look at focusworkspace:
# int oldtag = m->curtag; // oldtag = 0
# m->curtag = tag;        // m->curtag = 1
# m->ws = &workspaces[tag]; // m->ws = &workspaces[1]
# ...
# for (Client *fc = clients; fc; fc = fc->next) {
#     if (fc->workspace == oldtag)
#         hide_window(dpy, fc->win);
# }
# done:
# arrange(m);
#
# WAIT! What if the user switches workspaces on Monitor 0?!
# How does Monitor 0 switch workspaces?
# On Monitor 0: m->ntags is 1! m->tags = [8].
# But what if the user presses Alt+1 while the focus is on Monitor 0?
# In focusworkspace:
# tag = 0 (Workspace 1).
# prev = selmon (Monitor 0).
# tagmap[0] == 1 -> tm->num == 1 (Monitor 1).
# m = Monitor 1. selmon = Monitor 1.
# What if Monitor 1 ALREADY had curtag == 0?
# if (m->curtag == tag) goto done!
# What happens in done?
# if (prev != m) { prev->dirty = 1; m->dirty = 1; arrange(prev); }
# arrange(m);
#
# WAIT! What if user is on Monitor 1 (eDP-1) and presses Alt+9?
# prev = selmon (Monitor 1).
# tag = 8 (Workspace 9).
# tagmap[8] == 0 -> tm->num == 0 (Monitor 0).
# m = Monitor 0. selmon = Monitor 0.
# On Monitor 0: m->curtag is ALREADY 8!
# if (m->curtag == tag) goto done!
# So it jumps to done!
#
# NOW LOOK AT THIS:
# What if the user DOES NOT HAVE tagmap pinning, or what if nunique was <= 1 when updategeom ran?!
