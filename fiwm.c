/* fiwm - BSP tiling window manager for X11
 * See LICENSE file for copyright and license details.
 *
 * "dwm com cerebro de bspwm" — minimalista, rapido, hackavel.
 *
 * Design:
 * - Single-file, dwm-style: edit, recompile, restart.
 * - BSP tree replaces dwm's stack layouts.  Every window is a leaf in a
 *   recursively partitioned binary tree.  Internal nodes hold split
 *   orientation (vertical/horizontal) and ratio.
 * - No bar / text rendering — avoids Xft/fontconfig.  Use an external
 *   status bar (polybar, dzen, etc.) via _NET_WM_STRUT.
 * - Xinerama multi-monitor; each monitor shows one workspace.
 * - Workspaces global (0..8), each owns its own BSP tree.
 * - EWMH close protocol (WM_DELETE_WINDOW) for clean shutdown.
 */

#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#define LENGTH(X)           (sizeof(X) / sizeof(X)[0])
#define MAX(A,B)            ((A) > (B) ? (A) : (B))
#define MIN(A,B)            ((A) < (B) ? (A) : (B))
#define CLEANMASK(mask)     (mask & ~(numlockmask|LockMask))

enum { SPLIT_VERTICAL, SPLIT_HORIZONTAL };
enum { COL_BORDER, COL_FOCUS, COL_BAR_BG, COL_BAR_FG, COL_BAR_HL };
enum { WORKSPACE_COUNT = 9 };
enum { BAR_HEIGHT = 18 };
enum { WM_PROTOCOLS, WM_DELETE_WINDOW, WM_STATE, WM_LAST };
enum { NET_WM_STATE, NET_WM_STATE_FULLSCREEN, NET_ACTIVE_WINDOW,
       NET_WM_WINDOW_TYPE, NET_WM_WINDOW_TYPE_DIALOG, NET_LAST };

typedef union {
    int i;
    unsigned int ui;
    float f;
    const void *v;
} Arg;

typedef struct {
    unsigned int mod;
    KeySym keysym;
    void (*func)(const Arg *);
    const Arg arg;
} Key;

typedef struct Client Client;
typedef struct Node Node;
typedef struct Workspace Workspace;
typedef struct Monitor Monitor;

struct Client {
    Window win;
    int x, y, w, h;
    int oldx, oldy, oldw, oldh;
    int is_floating;
    int is_fullscreen;
    int is_dialog;
    Client *next;
    Client *prev;
};

struct Node {
    int is_leaf;
    int split;
    float ratio;
    Client *client;
    Node *parent;
    Node *first;
    Node *second;
};

struct Workspace {
    Node *root;
    Node *focus;
    int client_count;
};

struct Monitor {
    int num;
    int x, y, w, h;
    int wx, wy, ww, wh;
    Workspace *ws;
    Window barwin;
    int curtag;
    int next_split;
    Monitor *next;
};

static void die(const char *fmt, ...) __attribute__((noreturn, format(printf,1,2)));
static void *ecalloc(size_t, size_t);
static void spawn(const Arg *);

static Node *node_new(void);
static Node *node_detach(Node *, Workspace *);
static Node *node_insert(Monitor *, Client *);
static void  node_rotate(Node *);
static Node *node_in_direction(Node *, int, int);
static Node *node_first_leaf(Node *);
static Node *node_find_client(Node *, Client *);
static void node_hide_foreach(Node *);
static void node_pool_reset(void);

static void arrange(Monitor *);
static void arrange_node(Node *, int, int, int, int, int);

static void manage(Window, XWindowAttributes *);
static void unmanage(Client *);
static void focus(Monitor *, Node *);
static Client *find_client(Window);
static void movemouse(const Arg *);
static void resizemouse(const Arg *);

static void drawbar(Monitor *);
static void createbars(void);
static void updategeom(void);
#ifdef XINERAMA
static int isuniquegeom(XineramaScreenInfo *, int, XineramaScreenInfo *);
#endif

static void keypress(XEvent *);
static void buttonpress(XEvent *);
static void enternotify(XEvent *);
static void maprequest(XEvent *);
static void unmapnotify(XEvent *);
static void destroywindow(XEvent *);
static void configurerequest(XEvent *);

static int xerror(Display *, XErrorEvent *);
static void quit(const Arg *);
static void killclient(const Arg *);
static void togglefullscreen(const Arg *);
static void togglefloating(const Arg *);
static void focusworkspace(const Arg *);
static void focusworkspace_next(const Arg *);
static void focusworkspace_prev(const Arg *);
static void rotatecmd(const Arg *);
static void focuscmd(const Arg *);
static void movecmd(const Arg *);
static void setlayoutcmd(const Arg *);

static void setup(void);
static void scan(void);
static void grabkeys(void);
static void run(void);
static void cleanup(void);

static const int gappx           = 10;
static const char *termcmd[]     = { "alacritty", NULL };
static const char *menucmd[]     = {"/bin/sh", "-c", "/home/$USER/dotfile/dmenuscript", NULL};
static const float default_ratio = 0.5f;
static const char *colors[]      = { "#222222", "#7c3aed", "#000000", "#ffffff", "#444444" };
static const unsigned int MODKEY = Mod1Mask;
static Key keys[] = {
    { MODKEY,              XK_w,      spawn,          {.v = termcmd} },
    { MODKEY,              XK_d,      spawn,          {.v = menucmd} },
    { MODKEY,              XK_q,      killclient,     {0} },
    { MODKEY|ShiftMask,    XK_q,      quit,           {0} },
    { MODKEY,              XK_f,      togglefullscreen, {0} },
    { MODKEY,              XK_space,  togglefloating, {0} },
    { MODKEY,              XK_r,      rotatecmd,      {0} },
    { MODKEY,              XK_b,      setlayoutcmd,   {.i = SPLIT_VERTICAL} },
    { MODKEY,              XK_v,      setlayoutcmd,   {.i = SPLIT_HORIZONTAL} },
    { MODKEY,              XK_a,      focusworkspace_prev, {0} },
    { MODKEY,              XK_s,      focusworkspace_next, {0} },
    { MODKEY,              XK_h,      focuscmd,       {.i = (SPLIT_VERTICAL<<1)|0} },
    { MODKEY,              XK_l,      focuscmd,       {.i = (SPLIT_VERTICAL<<1)|1} },
    { MODKEY,              XK_k,      focuscmd,       {.i = (SPLIT_HORIZONTAL<<1)|0} },
    { MODKEY,              XK_j,      focuscmd,       {.i = (SPLIT_HORIZONTAL<<1)|1} },
    { MODKEY|ShiftMask,    XK_h,      movecmd,        {.i = 100 + ((SPLIT_VERTICAL<<1)|0)} },
    { MODKEY|ShiftMask,    XK_l,      movecmd,        {.i = 100 + ((SPLIT_VERTICAL<<1)|1)} },
    { MODKEY|ShiftMask,    XK_k,      movecmd,        {.i = 100 + ((SPLIT_HORIZONTAL<<1)|0)} },
    { MODKEY|ShiftMask,    XK_j,      movecmd,        {.i = 100 + ((SPLIT_HORIZONTAL<<1)|1)} },
    { MODKEY,              XK_1,      focusworkspace, {.i = 0} },
    { MODKEY,              XK_2,      focusworkspace, {.i = 1} },
    { MODKEY,              XK_3,      focusworkspace, {.i = 2} },
    { MODKEY,              XK_4,      focusworkspace, {.i = 3} },
    { MODKEY,              XK_5,      focusworkspace, {.i = 4} },
    { MODKEY,              XK_6,      focusworkspace, {.i = 5} },
    { MODKEY,              XK_7,      focusworkspace, {.i = 6} },
    { MODKEY,              XK_8,      focusworkspace, {.i = 7} },
    { MODKEY,              XK_9,      focusworkspace, {.i = 8} },
    { MODKEY|ShiftMask,    XK_1,      movecmd,        {.i = 0} },
    { MODKEY|ShiftMask,    XK_2,      movecmd,        {.i = 1} },
    { MODKEY|ShiftMask,    XK_3,      movecmd,        {.i = 2} },
    { MODKEY|ShiftMask,    XK_4,      movecmd,        {.i = 3} },
    { MODKEY|ShiftMask,    XK_5,      movecmd,        {.i = 4} },
    { MODKEY|ShiftMask,    XK_6,      movecmd,        {.i = 5} },
    { MODKEY|ShiftMask,    XK_7,      movecmd,        {.i = 6} },
    { MODKEY|ShiftMask,    XK_8,      movecmd,        {.i = 7} },
    { MODKEY|ShiftMask,    XK_9,      movecmd,        {.i = 8} },
};

static Display *dpy;
static Screen *scr;
static Window root;
static Monitor *mons;
static Monitor *selmon;
static Workspace workspaces[WORKSPACE_COUNT];
static Client *clients;
static Atom wmatom[WM_LAST];
static Atom netatom[NET_LAST];
static int running = 1;
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask;

/* node pool — avoids malloc/free at runtime */
enum { NODEPOOL = 256 };
static Node  nodepool[NODEPOOL];
static int   nodeidx;
static Node *nodefreelist;

/*──── utility ───────────────────────────────────────────────────────────────*/

void die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

void *ecalloc(size_t nmemb, size_t size)
{
    void *p;
    if (!(p = calloc(nmemb, size)))
        die("fiwm: calloc failed\n");
    return p;
}

void spawn(const Arg *arg)
{
    const char **argv = (const char **)arg->v;
    if (!argv || !argv[0])
        return;
    if (fork() == 0) {
        if (fork() == 0) {
            if (dpy)
                close(ConnectionNumber(dpy));
            setsid();
            execvp(argv[0], (char *const *)argv);
            fprintf(stderr, "fiwm: execvp %s: %s\n", argv[0], strerror(errno));
            _exit(EXIT_FAILURE);
        }
        _exit(EXIT_SUCCESS);
    }
    wait(NULL);
}

/*──── tree operations ───────────────────────────────────────────────────────*/

Node *node_new(void)
{
    Node *n;
    if (nodefreelist) {
        n = nodefreelist;
        nodefreelist = n->parent;
    } else if (nodeidx < NODEPOOL) {
        n = &nodepool[nodeidx++];
    } else {
        die("fiwm: node pool exhausted");
    }
    n->is_leaf = 1;
    n->split   = 0;
    n->ratio   = default_ratio;
    n->client  = NULL;
    n->parent  = NULL;
    n->first   = NULL;
    n->second  = NULL;
    return n;
}

static void node_free(Node *n)
{
    if (!n) return;
    n->parent = nodefreelist;
    nodefreelist = n;
}

void node_pool_reset(void)
{
    nodeidx      = 0;
    nodefreelist = NULL;
}

/*
 * Detach leaf `n` from the tree.  The sibling leaf/subtree moves up.
 * Returns focus candidate (sibling leaf or its leftmost leaf), or NULL
 * if the tree becomes empty.  Updates ws->root when the root changes.
 * Caller must free `n` afterwards.
 */
Node *node_detach(Node *n, Workspace *ws)
{
    Node *parent, *sibling, *grandparent;

    if (!n || !n->is_leaf) return NULL;

    parent = n->parent;
    if (!parent) {
        ws->root = NULL;
        return NULL;
    }

    sibling   = (parent->first == n) ? parent->second : parent->first;
    grandparent = parent->parent;

    sibling->parent = grandparent;
    if (grandparent) {
        if (grandparent->first == parent)
            grandparent->first = sibling;
        else
            grandparent->second = sibling;
    } else {
        ws->root = sibling;
    }

    node_free(parent);

    if (sibling->is_leaf)
        return sibling;
    return node_first_leaf(sibling);
}

/*
 * Insert `c` into the workspace tree by splitting the focused leaf.
 * If the workspace is empty the client becomes the leaf root.
 */
Node *node_insert(Monitor *m, Client *c)
{
    Workspace *ws = m->ws;
    Node *focus = ws->focus;
    Node *newnode, *newleaf;

    newleaf = node_new();
    newleaf->client = c;

    if (!focus || ws->client_count == 0) {
        ws->root = newleaf;
        ws->client_count = 1;
        return newleaf;
    }

    newnode = node_new();
    newnode->is_leaf = 0;

    if (m->next_split >= 0) {
        newnode->split = m->next_split;
        m->next_split  = -1;
    } else {
        newnode->split = focus->parent
                         ? !focus->parent->split
                         : SPLIT_VERTICAL;
    }
    newnode->ratio  = default_ratio;
    newnode->parent = focus->parent;
    newnode->first  = focus;
    newnode->second = newleaf;
    focus->parent   = newnode;
    newleaf->parent = newnode;

    if (newnode->parent) {
        if (newnode->parent->first == focus)
            newnode->parent->first = newnode;
        else
            newnode->parent->second = newnode;
    } else {
        ws->root = newnode;
    }

    ws->client_count++;
    return newleaf;
}

void node_rotate(Node *n)
{
    Node *tmp;
    if (!n || !n->is_leaf || !n->parent) return;
    n = n->parent;
    tmp    = n->first;
    n->first  = n->second;
    n->second = tmp;
}

/*
 * BSP directional leaf search.
 *
 * Walk ancestors until a node matching `orient` is found where current
 * is on the *opposite* side from `dir`.  Cross to the wanted side and
 * descend.
 *
 * orient : SPLIT_VERTICAL  => h/l (left / right)
 *          SPLIT_HORIZONTAL => k/j (up / down)
 * dir    : 0 => first child  (left / up)
 *          1 => second child (right / down)
 */
Node *node_in_direction(Node *n, int orient, int dir)
{
    Node *p, *c;
    int want_first = (dir == 0);

    if (!n || !n->is_leaf) return NULL;

    p = n;
    while (p->parent) {
        if (p->parent->split == orient) {
            int on_first = (p->parent->first == p);
            if (on_first != want_first) {
                c = want_first ? p->parent->first : p->parent->second;

                /* relative orthogonal position */
                int sub_pos = -1;
                Node *q = n;
                while (q != p->parent) {
                    if (q->parent && q->parent->split != orient)
                        sub_pos = (q->parent->first == q) ? 0 : 1;
                    q = q->parent;
                }

                /* descend — preserve orthogonal position when possible */
                while (!c->is_leaf) {
                    if (c->split != orient && sub_pos >= 0)
                        c = sub_pos ? c->second : c->first;
                    else
                        c = dir ? c->first : c->second;
                }
                return c;
            }
        }
        p = p->parent;
    }
    return NULL;
}

Node *node_first_leaf(Node *n)
{
    if (!n) return NULL;
    while (!n->is_leaf) n = n->first;
    return n;
}

Node *node_find_client(Node *n, Client *c)
{
    Node *f;
    if (!n) return NULL;
    if (n->is_leaf && n->client == c)
        return n;
    if (!n->is_leaf) {
        f = node_find_client(n->first, c);
        return f ? f : node_find_client(n->second, c);
    }
    return NULL;
}

void node_hide_foreach(Node *n)
{
    if (!n) return;
    if (n->is_leaf) {
        if (n->client)
            XMoveWindow(dpy, n->client->win, -9999, -9999);
    } else {
        node_hide_foreach(n->first);
        node_hide_foreach(n->second);
    }
}
/*──── layout ────────────────────────────────────────────────────────────────*/

/*
 * Recursive BSP layout.  Partitions (x,y,w,h) along each internal node's
 * split orientation and ratio, assigns pixel-perfect geometry to leaves.
 * `gap` is the inner-gap pixel spacing at this recursion level.
 */
void arrange_node(Node *n, int x, int y, int w, int h, int gap)
{
    Client *c;
    if (!n) return;

    if (n->is_leaf) {
        c = n->client;
        if (!c || c->is_fullscreen || c->is_floating)
            return;
        c->x = x;
        c->y = y;
        c->w = w;
        c->h = h;
        XMoveResizeWindow(dpy, c->win, x, y, w, h);
        return;
    }

    if (n->split == SPLIT_VERTICAL) {
        double ratio = (double)n->ratio;
        int sw  = (int)(w * ratio);
        int fw  = sw;
        int sw2 = w - sw;
        if (gap > 0) { fw -= gap/2; sw2 -= gap/2; }
        if (fw  < 1) fw  = 1;
        if (sw2 < 1) sw2 = 1;
        arrange_node(n->first,  x, y, fw, h, gap);
        arrange_node(n->second, x + fw + gap, y, sw2, h, gap);
    } else {
        double ratio = (double)n->ratio;
        int sh  = (int)(h * ratio);
        int fh  = sh;
        int sh2 = h - sh;
        if (gap > 0) { fh -= gap/2; sh2 -= gap/2; }
        if (fh  < 1) fh  = 1;
        if (sh2 < 1) sh2 = 1;
        arrange_node(n->first,  x, y, w, fh, gap);
        arrange_node(n->second, x, y + fh + gap, w, sh2, gap);
    }
}

void arrange(Monitor *m)
{
    int ogap, igap;

    if (!m) return;

    ogap = (gappx > 0) ? gappx : 0;
    igap = (gappx > 0) ? gappx : 0;

    /* position bar with margin on all sides igual ao gap das janelas */
    if (m->barwin)
        XMoveResizeWindow(dpy, m->barwin,
                          m->x + ogap, m->y + ogap,
                          m->w - 2 * ogap, BAR_HEIGHT);

    /* work area: abaixo da barra + gap entre barra e janelas */
    m->wx = m->x + ogap;
    m->wy = m->y + ogap + BAR_HEIGHT + ogap;
    m->ww = m->w - 2 * ogap;
    m->wh = m->h - BAR_HEIGHT - 3 * ogap;

    if (m->ws->root)
        arrange_node(m->ws->root, m->wx, m->wy, m->ww, m->wh, igap);

    if (m->ws->focus && m->ws->focus->client) {
        Client *c = m->ws->focus->client;
        if (c->is_fullscreen)
            XMoveResizeWindow(dpy, c->win, m->x, m->y, m->w, m->h);
        XRaiseWindow(dpy, c->win);
    }

    drawbar(m);
}

/*──── window management ─────────────────────────────────────────────────────*/

void manage(Window w, XWindowAttributes *wa)
{
    Client *c;
    XSetWindowAttributes swa;
    Atom actual, *atoms;
    int format;
    unsigned long n, left;
    unsigned char *data;

    c = ecalloc(1, sizeof(Client));
    c->win = w;
    c->x   = wa->x;
    c->y   = wa->y;
    c->w   = wa->width;
    c->h   = wa->height;

    /* detect dialog windows — make them floating */
    if (XGetWindowProperty(dpy, w, netatom[NET_WM_WINDOW_TYPE], 0L, 2L,
                           False, XA_ATOM, &actual, &format,
                           &n, &left, &data) == Success && data) {
        atoms = (Atom *)data;
        for (unsigned long i = 0; i < n; i++)
            if (atoms[i] == netatom[NET_WM_WINDOW_TYPE_DIALOG])
                c->is_dialog = c->is_floating = 1;
        XFree(data);
    }

    /* link into global list */
    if (clients) clients->prev = c;
    c->next = clients;
    c->prev = NULL;
    clients = c;

    /* event mask */
    swa.event_mask = EnterWindowMask | FocusChangeMask | PropertyChangeMask |
                     ButtonPressMask;
    XChangeWindowAttributes(dpy, w, CWEventMask, &swa);

    /* grab Mod+click for move/resize on floating windows */
    XGrabButton(dpy, Button1, MODKEY, w, False, ButtonPressMask,
                GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(dpy, Button3, MODKEY, w, False, ButtonPressMask,
                GrabModeAsync, GrabModeAsync, None, None);

    /* insert into BSP tree (unless floating) */
    if (!c->is_floating) {
        Node *nl = node_insert(selmon, c);
        XMapWindow(dpy, w);
        if (nl) focus(selmon, nl);
        arrange(selmon);
        return;
    }

    XMapWindow(dpy, w);
}

void unmanage(Client *c)
{
    Monitor *m;
    Node *n, *newfocus;

    if (!c) return;

    /* find & remove from workspace tree */
    for (m = mons; m; m = m->next) {
        n = node_find_client(m->ws->root, c);
        if (!n) continue;
        newfocus = node_detach(n, m->ws);
        if (newfocus)
            m->ws->focus = newfocus;
        else
            m->ws->focus = NULL;
        node_free(n);
        m->ws->client_count--;
        arrange(m);
        break;
    }

    /* unlink from global list */
    if (c->next) c->next->prev = c->prev;
    if (c->prev) c->prev->next = c->next;
    else         clients = c->next;

    XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
    free(c);
}

void focus(Monitor *m, Node *n)
{
    if (!m || !n || !n->client) return;

    m->ws->focus = n;
    XSetInputFocus(dpy, n->client->win, RevertToPointerRoot, CurrentTime);
    XRaiseWindow(dpy, n->client->win);
}

Client *find_client(Window w)
{
    Client *c;
    for (c = clients; c; c = c->next)
        if (c->win == w) return c;
    return NULL;
}

void movemouse(const Arg *arg)
{
    (void)arg;
    Client *c;
    int di, ox, oy;
    unsigned int du;
    Window dw;
    XEvent ev;

    if (!selmon || !selmon->ws->focus || !(c = selmon->ws->focus->client))
        return;
    if (!c->is_floating) return;

    if (XGrabPointer(dpy, root, False,
                     PointerMotionMask | ButtonReleaseMask,
                     GrabModeAsync, GrabModeAsync, None, None,
                     CurrentTime) != GrabSuccess)
        return;

    XQueryPointer(dpy, root, &dw, &dw, &ox, &oy, &di, &di, &du);
    ox -= c->x; oy -= c->y;

    for (;;) {
        XNextEvent(dpy, &ev);
        switch (ev.type) {
        case MotionNotify:
            XMoveWindow(dpy, c->win,
                        ev.xmotion.x_root - ox,
                        ev.xmotion.y_root - oy);
            break;
        case ButtonRelease:
            XMoveWindow(dpy, c->win,
                        ev.xbutton.x_root - ox,
                        ev.xbutton.y_root - oy);
            XUngrabPointer(dpy, CurrentTime);
            c->x = ev.xbutton.x_root - ox;
            c->y = ev.xbutton.y_root - oy;
            return;
        }
    }
}

void resizemouse(const Arg *arg)
{
    (void)arg;
    Client *c;
    int di, mx, my;
    unsigned int du;
    Window dw;
    XEvent ev;

    if (!selmon || !selmon->ws->focus || !(c = selmon->ws->focus->client))
        return;
    if (!c->is_floating) return;

    if (XGrabPointer(dpy, root, False,
                     PointerMotionMask | ButtonReleaseMask,
                     GrabModeAsync, GrabModeAsync, None, None,
                     CurrentTime) != GrabSuccess)
        return;

    XQueryPointer(dpy, root, &dw, &dw, &di, &di, &mx, &my, &du);

    for (;;) {
        XNextEvent(dpy, &ev);
        switch (ev.type) {
        case MotionNotify:
            XResizeWindow(dpy, c->win,
                          MAX(1, ev.xmotion.x_root - c->x),
                          MAX(1, ev.xmotion.y_root - c->y));
            break;
        case ButtonRelease:
            c->w = MAX(1, ev.xbutton.x_root - c->x);
            c->h = MAX(1, ev.xbutton.y_root - c->y);
            XResizeWindow(dpy, c->win, c->w, c->h);
            XUngrabPointer(dpy, CurrentTime);
            return;
        }
    }
}

/*──── monitor / workspace ───────────────────────────────────────────────────*/

#ifdef XINERAMA
int isuniquegeom(XineramaScreenInfo *unique, int n, XineramaScreenInfo *info)
{
    int i;
    for (i = 0; i < n; i++)
        if (unique[i].x_org  == info->x_org  &&
            unique[i].y_org  == info->y_org  &&
            unique[i].width  == info->width  &&
            unique[i].height == info->height)
            return 0;
    return 1;
}
#endif

void updategeom(void)
{
    Monitor *m, *newmons = NULL, *tail = NULL;
    int nmon = 0;

#ifdef XINERAMA
    if (XineramaIsActive(dpy)) {
        int i, j, n;
        XineramaScreenInfo *info = XineramaQueryScreens(dpy, &n);
        XineramaScreenInfo *unique = ecalloc(n, sizeof(XineramaScreenInfo));

        for (i = 0, j = 0; i < n; i++)
            if (isuniquegeom(unique, j, &info[i]))
                memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));

        for (i = 0; i < j; i++) {
            m = ecalloc(1, sizeof(Monitor));
            m->num   = nmon++;
            m->x     = unique[i].x_org;
            m->y     = unique[i].y_org;
            m->w     = unique[i].width;
            m->h     = unique[i].height;
            m->curtag = i < WORKSPACE_COUNT ? i : 0;
            m->ws    = &workspaces[m->curtag];
            m->next_split = -1;
            if (!newmons) newmons = m;
            else          tail->next = m;
            tail = m;
        }
        XFree(info);
        free(unique);
    } else
#endif
    {
        m = ecalloc(1, sizeof(Monitor));
        m->num   = 0;
        m->x     = 0;
        m->y     = 0;
        m->w     = DisplayWidth(dpy, XDefaultScreen(dpy));
        m->h     = DisplayHeight(dpy, XDefaultScreen(dpy));
        m->curtag = 0;
        m->ws    = &workspaces[0];
        m->next_split = -1;
        newmons = m;
        nmon = 1;
    }

    while (mons) {
        m = mons->next;
        free(mons);
        mons = m;
    }
    mons  = newmons;
    selmon = mons;
}

void focusworkspace(const Arg *arg)
{
    Monitor *m, *om;
    Workspace *oldws;
    int tag = arg->i;

    if (tag < 0 || tag >= WORKSPACE_COUNT || !selmon)
        return;

    m = selmon;
    oldws = m->ws;

    /* hide windows leaving this monitor (move off-screen) */
    node_hide_foreach(oldws->root);

    /* if another monitor already shows this tag, swap */
    om = NULL;
    for (om = mons; om; om = om->next) {
        if (om != m && om->curtag == tag) {
            Workspace *tmpws = m->ws;
            int tmptag = m->curtag;
            m->curtag = tag;
            m->ws     = &workspaces[tag];
            om->curtag = tmptag;
            om->ws     = tmpws;
            break;
        }
    }
    if (!om) {
        m->curtag = tag;
        m->ws     = &workspaces[tag];
    }

    if (m->ws->focus && m->ws->focus->client)
        focus(m, m->ws->focus);
    else if (m->ws->root) {
        Node *first = node_first_leaf(m->ws->root);
        if (first)
            focus(m, first);
    }

    arrange(m);
}

void focusworkspace_next(const Arg *arg)
{
    (void)arg;
    if (!selmon) return;
    int tag = (selmon->curtag + 1) % WORKSPACE_COUNT;
    Arg a = {.i = tag};
    focusworkspace(&a);
}

void focusworkspace_prev(const Arg *arg)
{
    (void)arg;
    if (!selmon) return;
    int tag = (selmon->curtag - 1 + WORKSPACE_COUNT) % WORKSPACE_COUNT;
    Arg a = {.i = tag};
    focusworkspace(&a);
}

/*──── bar ───────────────────────────────────────────────────────────────────*/

void drawbar(Monitor *m)
{
    char buf[32];
    int i, x;
    XColor bg, fg, hl;
    Colormap cmap;
    GC gc;
    Window win;
    XWindowAttributes wa;
    int sw; /* button slot width */

    if (!m || !m->barwin)
        return;

    win = m->barwin;
    cmap = DefaultColormap(dpy, XDefaultScreen(dpy));
    XGetWindowAttributes(dpy, win, &wa);
    sw = wa.width / WORKSPACE_COUNT;

    XAllocNamedColor(dpy, cmap, colors[COL_BAR_BG], &bg, &bg);
    XAllocNamedColor(dpy, cmap, colors[COL_BAR_FG], &fg, &fg);
    XAllocNamedColor(dpy, cmap, colors[COL_BAR_HL], &hl, &hl);

    gc = XCreateGC(dpy, win, 0, NULL);

    XSetForeground(dpy, gc, bg.pixel);
    XFillRectangle(dpy, win, gc, 0, 0, wa.width, wa.height);

    for (i = 0; i < WORKSPACE_COUNT; i++) {
        x = i * sw;
        snprintf(buf, sizeof(buf), "%d", i + 1);

        XSetForeground(dpy, gc, (m->curtag == i) ? fg.pixel : hl.pixel);
        XDrawString(dpy, win, gc, x + sw/2 - 4, wa.height/2 + 5, buf, strlen(buf));
    }

    XFreeGC(dpy, gc);
    XSync(dpy, False);
}

void createbars(void)
{
    Monitor *m;
    XSetWindowAttributes wa;

    for (m = mons; m; m = m->next) {
        wa.override_redirect = True;
        wa.background_pixel = 0;
        wa.event_mask = ExposureMask | ButtonPressMask;

        m->barwin = XCreateWindow(dpy, root,
                                  0, 0, 1, 1, 0,
                                  DefaultDepth(dpy, XDefaultScreen(dpy)),
                                  InputOutput,
                                  DefaultVisual(dpy, XDefaultScreen(dpy)),
                                  CWOverrideRedirect | CWEventMask,
                                  &wa);

        XMapWindow(dpy, m->barwin);
        XLowerWindow(dpy, m->barwin);
    }
}

/*──── event handlers ────────────────────────────────────────────────────────*/

void keypress(XEvent *e)
{
    XKeyEvent *ev = &e->xkey;
    KeySym keysym = XkbKeycodeToKeysym(dpy, ev->keycode, 0, 0);

    for (size_t i = 0; i < LENGTH(keys); i++)
        if (keysym == keys[i].keysym &&
            CLEANMASK(keys[i].mod) == CLEANMASK(ev->state))
            { keys[i].func(&keys[i].arg); return; }
}

void buttonpress(XEvent *e)
{
    Client *c = find_client(e->xbutton.window);
    Monitor *m;
    Node *n;

    /* bar click — switch workspace */
    for (m = mons; m; m = m->next) {
        if (e->xbutton.window == m->barwin) {
            XWindowAttributes wa;
            if (XGetWindowAttributes(dpy, m->barwin, &wa)) {
                int sw = wa.width / WORKSPACE_COUNT;
                int tag = e->xbutton.x / sw;
                if (tag >= 0 && tag < WORKSPACE_COUNT) {
                    Arg a = {.i = tag};
                    focusworkspace(&a);
                }
            }
            return;
        }
    }

    /* Mod+click on client — move/resize floating */
    if (c && (e->xbutton.state & MODKEY)) {
        for (m = mons; m; m = m->next) {
            n = node_find_client(m->ws->root, c);
            if (n) { focus(m, n); break; }
        }
        if (e->xbutton.button == Button1)
            movemouse(NULL);
        else if (e->xbutton.button == Button3)
            resizemouse(NULL);
        return;
    }

    /* window click — focus */
    if (!c) return;
    for (m = mons; m; m = m->next) {
        n = node_find_client(m->ws->root, c);
        if (n) { focus(m, n); arrange(m); return; }
    }
}

void enternotify(XEvent *e)
{
    XCrossingEvent *ev = &e->xcrossing;
    Client *c;
    Monitor *m;
    Node *n;

    if (ev->mode != NotifyNormal || ev->detail == NotifyInferior)
        return;

    c = find_client(ev->window);
    if (!c) return;

    for (m = mons; m; m = m->next) {
        n = node_find_client(m->ws->root, c);
        if (n) { focus(m, n); arrange(m); return; }
    }
}

void maprequest(XEvent *e)
{
    XWindowAttributes wa;

    if (!XGetWindowAttributes(dpy, e->xmaprequest.window, &wa))
        return;
    if (wa.override_redirect)
        return;

    manage(e->xmaprequest.window, &wa);
}

void unmapnotify(XEvent *e)
{
    Client *c;
    if (!(c = find_client(e->xunmap.window)))
        return;
    if (e->xunmap.send_event)
        return;
    unmanage(c);
}

void destroywindow(XEvent *e)
{
    Client *c;
    if ((c = find_client(e->xdestroywindow.window)))
        unmanage(c);
}

void configurerequest(XEvent *e)
{
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    Client *c = find_client(ev->window);
    XWindowChanges wc;

    wc.x           = ev->x;
    wc.y           = ev->y;
    wc.width       = ev->width;
    wc.height      = ev->height;
    wc.border_width = ev->border_width;
    wc.sibling     = ev->above;
    wc.stack_mode  = ev->detail;

    if (c && c->is_floating) {
        c->x = ev->x; c->y = ev->y;
        c->w = ev->width; c->h = ev->height;
        XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
    } else if (!c) {
        XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
    }
}

/*──── error handling ────────────────────────────────────────────────────────*/

int xerror(Display *dpy_, XErrorEvent *ee)
{
    (void)dpy_;
    if (ee->error_code == BadWindow ||
        ee->error_code == BadDrawable ||
        ee->error_code == BadColor)
        return 0;
    return 0;
}

/*──── commands ──────────────────────────────────────────────────────────────*/

void quit(const Arg *arg)
{
    (void)arg;
    running = 0;
}

void killclient(const Arg *arg)
{
    Client *c;
    (void)arg;
    XEvent ev;

    if (!selmon || !selmon->ws->focus)
        return;
    c = selmon->ws->focus->client;
    if (!c) return;

    ev.type = ClientMessage;
    ev.xclient.window = c->win;
    ev.xclient.message_type = wmatom[WM_PROTOCOLS];
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = wmatom[WM_DELETE_WINDOW];
    ev.xclient.data.l[1] = CurrentTime;
    XSendEvent(dpy, c->win, False, NoEventMask, &ev);
}

void togglefullscreen(const Arg *arg)
{
    Client *c;
    (void)arg;
    if (!selmon || !selmon->ws->focus || !(c = selmon->ws->focus->client))
        return;

    c->is_fullscreen = !c->is_fullscreen;
    if (c->is_fullscreen) {
        c->oldx = c->x; c->oldy = c->y;
        c->oldw = c->w; c->oldh = c->h;
        XMoveResizeWindow(dpy, c->win, selmon->x, selmon->y,
                          selmon->w, selmon->h);
        XRaiseWindow(dpy, c->win);
    } else {
        arrange(selmon);
    }
}

void togglefloating(const Arg *arg)
{
    Client *c;
    (void)arg;
    if (!selmon || !selmon->ws->focus || !(c = selmon->ws->focus->client))
        return;

    c->is_floating = !c->is_floating;
    arrange(selmon);
}

void rotatecmd(const Arg *arg)
{
    (void)arg;
    if (!selmon || !selmon->ws->focus) return;
    node_rotate(selmon->ws->focus);
    arrange(selmon);
}

void focuscmd(const Arg *arg)
{
    int orient, dir;

    if (!selmon || !selmon->ws->focus) return;

    orient = arg->i >> 1;
    dir    = arg->i & 1;

    Node *target = node_in_direction(selmon->ws->focus, orient, dir);
    if (target) {
        focus(selmon, target);
        arrange(selmon);
    }
}

void movecmd(const Arg *arg)
{
    Node *n, *target;
    Workspace *ws;
    Client *tmpc;
    int orient, dir, raw;

    if (!selmon || !(n = selmon->ws->focus)) return;
    ws = selmon->ws;

    raw = arg->i;

    /* if raw is a workspace index (0..8), move client there */
    if (raw >= 0 && raw < WORKSPACE_COUNT) {
        Client *c = n->client;
        int tag = raw;
        if (!c) return;

        Node *newfocus = node_detach(n, ws);
        ws->focus = newfocus;
        ws->client_count--;
        node_free(n);

        Workspace *tws = &workspaces[tag];
        Node *nl = node_new();
        nl->client = c;

        if (!tws->root) {
            tws->root = nl;
            tws->focus = nl;
        } else {
            Node *ff = tws->focus ? tws->focus : node_first_leaf(tws->root);
            Node *nn = node_new();
            nn->is_leaf = 0;
            nn->split = SPLIT_VERTICAL;
            nn->ratio = default_ratio;
            nn->first  = ff;
            nn->second = nl;
            nn->parent = ff->parent;
            if (ff->parent) {
                if (ff->parent->first == ff)
                    ff->parent->first = nn;
                else
                    ff->parent->second = nn;
            } else {
                tws->root = nn;
            }
            ff->parent = nn;
            nl->parent = nn;
            tws->focus = nl;
        }
        tws->client_count++;
        arrange(selmon);
        return;
    }

    /* directional move (swap clients) */
    raw -= 100;
    orient = raw >> 1;
    dir    = raw & 1;

    target = node_in_direction(n, orient, dir);
    if (!target || !target->is_leaf || !target->client)
        return;

    tmpc = n->client;
    n->client = target->client;
    target->client = tmpc;

    focus(selmon, target);
    arrange(selmon);
}

void setlayoutcmd(const Arg *arg)
{
    if (selmon) selmon->next_split = arg->i;
}

/*──── setup / teardown ──────────────────────────────────────────────────────*/

void grabkeys(void)
{
    unsigned int i, j;
    KeySym keysym;
    unsigned int modifiers[] = { 0, LockMask, 0, LockMask };

    /* resolve numlock */
    {
        XModifierKeymap *modmap = XGetModifierMapping(dpy);
        for (i = 0; i < 8; i++)
            for (j = 0; (int)j < modmap->max_keypermod; j++)
                if (modmap->modifiermap[i * modmap->max_keypermod + j]
                    == XKeysymToKeycode(dpy, XK_Num_Lock))
                    numlockmask = (1 << i);
        XFreeModifiermap(modmap);
    }

    modifiers[2] = numlockmask;
    modifiers[3] = numlockmask | LockMask;

    XUngrabKey(dpy, AnyKey, AnyModifier, root);

    for (i = 0; i < LENGTH(keys); i++) {
        keysym = XKeysymToKeycode(dpy, keys[i].keysym);
        for (j = 0; j < LENGTH(modifiers); j++)
            XGrabKey(dpy, keysym, keys[i].mod | modifiers[j], root, True,
                     GrabModeAsync, GrabModeAsync);
    }
}

void scan(void)
{
    unsigned int i, n;
    Window d1, d2, *wins;
    XWindowAttributes wa;

    if (XQueryTree(dpy, root, &d1, &d2, &wins, &n)) {
        for (i = 0; i < n; i++) {
            if (!XGetWindowAttributes(dpy, wins[i], &wa) ||
                wa.override_redirect ||
                XGetTransientForHint(dpy, wins[i], &d1))
                continue;
            if (wa.map_state == IsViewable)
                manage(wins[i], &wa);
        }
        XFree(wins);
    }
}

void setup(void)
{
    unsigned int i;
    XSetWindowAttributes wa;
    Atom supported[] = {
        netatom[NET_WM_STATE],
        netatom[NET_WM_STATE_FULLSCREEN],
        netatom[NET_ACTIVE_WINDOW],
    };

    dpy = XOpenDisplay(NULL);
    if (!dpy) die("fiwm: cannot open display\n");
    scr  = XScreenOfDisplay(dpy, XDefaultScreen(dpy));
    root = RootWindow(dpy, XDefaultScreen(dpy));

    /* workspaces */
    for (i = 0; i < WORKSPACE_COUNT; i++) {
        workspaces[i].root  = NULL;
        workspaces[i].focus = NULL;
        workspaces[i].client_count = 0;
    }

    /* atoms */
    wmatom[WM_PROTOCOLS]              = XInternAtom(dpy, "WM_PROTOCOLS", False);
    wmatom[WM_DELETE_WINDOW]          = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    wmatom[WM_STATE]                  = XInternAtom(dpy, "WM_STATE", False);
    netatom[NET_WM_STATE]             = XInternAtom(dpy, "_NET_WM_STATE", False);
    netatom[NET_WM_STATE_FULLSCREEN]  = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
    netatom[NET_ACTIVE_WINDOW]        = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
    netatom[NET_WM_WINDOW_TYPE]       = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    netatom[NET_WM_WINDOW_TYPE_DIALOG]= XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);

    /* EWMH root props */
    XChangeProperty(dpy, root,
        XInternAtom(dpy, "_NET_SUPPORTED", False),
        XA_ATOM, 32, PropModeReplace,
        (unsigned char *)supported, LENGTH(supported));

    {
        unsigned long nd = WORKSPACE_COUNT;
        XChangeProperty(dpy, root,
            XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False),
            XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&nd, 1);
    }

    /* capture root events */
    wa.cursor = XCreateFontCursor(dpy, XC_left_ptr);
    wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask |
                    EnterWindowMask | ButtonPressMask |
                    StructureNotifyMask | PropertyChangeMask;
    XChangeWindowAttributes(dpy, root, CWEventMask | CWCursor, &wa);
    XSelectInput(dpy, root, wa.event_mask);

    /* error handler */
    xerrorxlib = XSetErrorHandler(xerror);

    /* monitors & existing windows */
    updategeom();
    createbars();
    grabkeys();
    scan();

    /* draw bars */
    for (Monitor *m = mons; m; m = m->next)
        drawbar(m);
}

void run(void)
{
    XEvent ev;
    while (running && !XNextEvent(dpy, &ev)) {
        switch (ev.type) {
        case KeyPress:         keypress(&ev);       break;
        case ButtonPress:      buttonpress(&ev);    break;
        case EnterNotify:      enternotify(&ev);    break;
        case MapRequest:       maprequest(&ev);     break;
        case UnmapNotify:      unmapnotify(&ev);    break;
        case DestroyNotify:    destroywindow(&ev);  break;
        case ConfigureRequest: configurerequest(&ev); break;
        case Expose:
            {
                Monitor *m;
                for (m = mons; m; m = m->next)
                    if (m->barwin == ev.xexpose.window)
                        drawbar(m);
            }
            break;
        }
    }
}

void cleanup(void)
{
    Monitor *m, *mtmp;
    Client *c;

    while (clients) {
        c = clients;
        XSelectInput(dpy, c->win, NoEventMask);
        XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
        clients = c->next;
        free(c);
    }

    node_pool_reset();

    m = mons;
    while (m) {
        if (m->barwin)
            XDestroyWindow(dpy, m->barwin);
        mtmp = m->next;
        free(m);
        m = mtmp;
    }

    XSetErrorHandler(xerrorxlib);
    XSync(dpy, False);
    XCloseDisplay(dpy);
}

/*──── main ──────────────────────────────────────────────────────────────────*/

int main(int argc, char *argv[])
{
    if (argc == 2 && !strcmp(argv[1], "-v")) {
        puts("fiwm 0.1");
        return 0;
    } else if (argc != 1) {
        die("usage: fiwm [-v]\n");
    }

    if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
        fputs("fiwm: no locale support\n", stderr);

    setup();
    run();
    cleanup();
    return 0;
}
