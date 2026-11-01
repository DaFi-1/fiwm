# Let's reproduce the exact bug described by the user:
# "quando eu coloco coisa no segundo monitor no workspace 9
# ai eu navego entre os workspaces o conteudo do workspace 9 aparece nos outros
# e some no workspace 9 mais aparece quando eu vou para ele"
#
# Workspaces: 0, 1, 2, 3, 4, 5, 6, 7, 8 (WS 1..9)
# tagmap: {0:1, 1:1, 2:1, 3:1, 4:1, 5:1, 6:1, 7:1, 8:0}
# Monitor 0: num=0, curtag=8 (Workspace 9)
# Monitor 1: num=1, curtag=0 (Workspace 1)
#
# User places a window on Workspace 9. Window c has:
# c->workspace = 8.
# Now user navigates between workspaces on Monitor 1!
# User is on Monitor 1, presses Alt+2 (tag 1).
# In focusworkspace:
# prev = selmon = Monitor 1
# m = selmon = Monitor 1 (tagmap[1] == 1)
# oldtag = m->curtag (tag 0)
# m->curtag = 1
# for fc in clients:
#     if fc->workspace == oldtag: hide_window(fc)  <- hides windows of oldtag (0)
# done:
# arrange(prev)
# arrange(m)
#
# Wait, what if user uses Alt+s (focusworkspace_step)?
# When user is on Workspace 8 (tag 7 on Monitor 1) and presses Alt+s (+1):
# tag = (7 + 1 + 9) % 9 = 8 (Workspace 9)!
# tagmap[8] == 0!
# So m becomes Monitor 0! selmon becomes Monitor 0!
# BUT on Monitor 0:
# m->curtag was ALREADY 8!
# So if (m->curtag == tag) triggers!
# What if user presses Alt+s AGAIN while on Monitor 0?
# selmon is Monitor 0, curtag is 8!
# tag = (8 + 1 + 9) % 9 = 0 (Workspace 1)!
# tagmap[0] == 1 -> Monitor 1!
# BUT on Monitor 1:
# What was Monitor 1's curtag? It was 7!
# So oldtag is 7.
# Monitor 1's curtag becomes 0.
# BUT what if the user pressed Alt+1..9 or moved a window?
