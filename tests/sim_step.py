# Let's inspect focusworkspace_step:
# void focusworkspace_step(const Arg *arg)
# {
#     if (!selmon) return;
#     int tag = (selmon->curtag + arg->i + WORKSPACE_COUNT) % WORKSPACE_COUNT;
#     Arg a = {.i = tag};
#     focusworkspace(&a);
# }
# Look closely at what happens:
# Monitor 0: tags = [8] (Workspace 9)
# Monitor 1: tags = [0, 1, 2, 3, 4, 5, 6, 7] (Workspaces 1-8)
#
# If selmon is on Monitor 0 (Workspace 9, tag 8):
# User presses Alt+s (step +1):
# tag = (8 + 1 + 9) % 9 = 0 (Workspace 1).
# focusworkspace is called with tag 0.
# tagmap[0] == 1 -> Monitor 1.
# selmon becomes Monitor 1!
# Monitor 1 changes curtag to 0.
#
# BUT what if selmon is Monitor 1 and its curtag is 8?
# Can Monitor 1 have curtag = 8?
# In tagmap:
# tagmap = { 1, 1, 1, 1, 1, 1, 1, 1, 0 }
#
# Now look at:
# void configurerequest(XEvent *e)
# void manage(Window w, XWindowAttributes *wa)
# c->workspace = selmon ? selmon->curtag : 0;
#
# When window spawns on Monitor 0, c->workspace = 8.
# What if user clicks on the bar?
# int sw = m->w / m->ntags;
# int clicked = xpos / sw;
# Arg a = {.i = m->tags[clicked]};
# focusworkspace(&a);
