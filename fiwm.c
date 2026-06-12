/* fiwm - BSP tiling WM (freestanding x86_64, _start entry) */
#include <X11/cursorfont.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif
#define LENGTH(X)           (sizeof(X) / sizeof(X)[0])
#define MAX(A,B)            ((A) > (B) ? (A) : (B))
#define MIN(A,B)            ((A) < (B) ? (A) : (B))
#define CLEANMASK(mask)     (mask & ~(numlockmask|LockMask))
#define SELCLIENT(v)        if (!selmon || !selmon->ws->focus || !(v = selmon->ws->focus->client)) return

enum { SPLIT_VERTICAL, SPLIT_HORIZONTAL };
enum { COL_BORDER_ACTIVE, COL_BORDER_INACTIVE, COL_BAR_BG, COL_BAR_FG, COL_BAR_HL };
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
    int tile_x, tile_y, tile_w, tile_h;
    unsigned is_floating : 1;
    unsigned is_fullscreen : 1;
    unsigned is_dialog : 1;
    Client *next;
    Client *prev;
};

struct Node {
    Client *client;
    Node *parent, *first, *second;
    unsigned is_leaf : 1;
    unsigned split   : 1;
};

struct Workspace {
    Node *root;
    Node *focus;
};

struct Monitor {
    Workspace *ws;
    Window barwin;
    Pixmap barpm;
    Monitor *next;
    int num, x, y, w, h;
    short curtag;
    short next_split;
    unsigned dirty : 1;
};
#define M_ARENA_MAX (-8)


static void die(const char *) __attribute__((noreturn));
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
static void arrange_floating(Monitor *);

static void manage(Window, XWindowAttributes *);
static void unmanage(Client *);
static void focus(Monitor *, Node *);
static Client *find_client(Window);
static void movemouse(const Arg *);
static void resizemouse(const Arg *);

static void drawbar(Monitor *);
static void createbars(void);
static void updategeom(void);
static void update_bar_visibility(Monitor *);
static Monitor *monitor_in_direction(Monitor *, int, int);
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
static void mask_children(Window w);

static const int gappx           = 10;
static const int borderpx        = 2;
static const char *termcmd[]     = { "alacritty", NULL };
static const char *menucmd[]     = {"/bin/sh", "-c", "/home/$USER/dotfile/dmenuscript", NULL};
static const float default_ratio = 0.5f;
static const char *colors[]      = {
    "#88775F",  /* COL_BORDER_ACTIVE   — borda da janela focada (ouro miasma) */
    "#2d2d2d",  /* COL_BORDER_INACTIVE — borda das janelas não focadas */
    "#1a1a1a",  /* COL_BAR_BG          — fundo da barra */
    "#c2c2b0",  /* COL_BAR_FG          — texto do workspace ativo na barra (bege miasma) */
    "#685742",  /* COL_BAR_HL          — texto dos workspaces inativos na barra (marrom miasma) */
};
static const int tagmap[WORKSPACE_COUNT] = {
    1, 1, 1, 1, 1, 1, 1, 1, 0
};
static const unsigned int MODKEY = Mod1Mask;

/* Autostart commands (run once at startup, before the event loop) */
static const char *autostart[][4] = {
    { "/bin/sh", "-c", "picom", NULL },
    { "/bin/sh", "-c", "nitrogen --restore", NULL },
    { "/bin/sh", "-c", "xset r rate 180 250", NULL },
    { "/bin/sh", "-c", "xrandr --output DP-0 --primary --mode 1920x1080 --rate 100 --pos 0x0 --output HDMI-0 --mode 1366x768 --rate 60 --right-of DP-0", NULL },
};

static Key keys[] = {
    { MODKEY,              XK_w,      spawn,          {.v = termcmd} },
    { MODKEY,              XK_d,      spawn,          {.v = menucmd} },
    { MODKEY,              XK_q,      killclient,     {0} },
    { MODKEY|ShiftMask,    XK_q,      quit,           {0} },
    { MODKEY,              XK_f,      togglefullscreen, {0} },
    { MODKEY|ShiftMask,    XK_f,      togglefloating,  {0} },
    { MODKEY,              XK_space,  togglefloating,  {0} },
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
static unsigned long border_active, border_inactive, bar_bg, bar_fg, bar_hl;
static GC bar_gc;

enum { NODEPOOL = 64 };
static Node  nodepool[NODEPOOL];
static int   nodeidx;
static Node *nodefreelist;
static Client *client_freelist;

static char **envp_global;
#define SYS_READ      0
#define SYS_WRITE     1
#define SYS_CLOSE     3
#define SYS_FORK     57
#define SYS_EXECVE   59
#define SYS_EXIT     60
#define SYS_WAIT4    61
#define SYS_SETSID  112

static inline long syscall3(long nr, long a1, long a2, long a3)
{
    long ret;
    __asm__ __volatile__("syscall"
                 : "=a"(ret)
                 : "a"(nr), "D"(a1), "S"(a2), "d"(a3)
                 : "rcx", "r11", "memory");
    return ret;
}

static inline __attribute__((noreturn)) void sys_exit(int code)
{
    __asm__ __volatile__("syscall"
                 :
                 : "a"(SYS_EXIT), "D"(code)
                 : "rcx", "r11", "memory");
    __builtin_unreachable();
}

#define sys_write(f,b,c)   syscall3(SYS_WRITE, (long)(f), (long)(b), (long)(c))
#define sys_fork()         syscall3(SYS_FORK, 0, 0, 0)
#define sys_execve(p,a,e)  syscall3(SYS_EXECVE, (long)(p), (long)(a), (long)(e))
#define sys_close(f)       syscall3(SYS_CLOSE, (long)(f), 0, 0)
#define sys_setsid()       syscall3(SYS_SETSID, 0, 0, 0)

static inline long sys_wait4(long pid, int *wstatus, int options, void *rusage)
{
    long ret;
    register long r10 __asm__("r10") = options;
    __asm__ __volatile__("syscall"
                 : "=a"(ret)
                 : "a"(SYS_WAIT4), "D"(pid), "S"(wstatus), "d"(rusage), "r"(r10)
                 : "rcx", "r11", "memory");
    return ret;
}
static size_t strlen(const char *s)
{
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static void *memset(void *s, int c, size_t n)
{
    unsigned char *p = s;
    for (size_t i = 0; i < n; i++) p[i] = (unsigned char)c;
    return s;
}

static int itoa(unsigned int n, char *buf, int size)
{
    char tmp[12];
    int i = 0, j = 0;

    if (n == 0) tmp[j++] = '0';
    while (n > 0 && j < 12) {
        tmp[j++] = '0' + (n % 10);
        n /= 10;
    }
    while (j > 0 && i < size - 1)
        buf[i++] = tmp[--j];
    buf[i] = '\0';
    return i;
}
#define HEAP_SIZE (1UL * 1024)          /* 1 KB — bump allocator */
static char heap[HEAP_SIZE];
static unsigned long heap_ptr;

static void *bump_calloc(size_t nmemb, size_t size)
{
    size_t total = nmemb * size;
    if (heap_ptr + total > HEAP_SIZE)
        return NULL;
    void *ptr = heap + heap_ptr;
    heap_ptr += total;
    memset(ptr, 0, total);
    return ptr;
}

static void free(void *p) { (void)p; }
static char *strchrnul(const char *s, int c)
{
    while (*s && *s != (char)c) s++;
    return (char *)s;
}

void die(const char *msg)
{
    sys_write(2, msg, strlen(msg));
    sys_exit(1);
}

void *ecalloc(size_t nmemb, size_t size)
{
    void *p = bump_calloc(nmemb, size);
    if (!p) die("fiwm: calloc failed\n");
    return p;
}

static void try_exec(const char *file, char *const argv[])
{
    char buf[1024];
    const char *path;
    const char *p;
    int has_slash;

    has_slash = (strchrnul(file, '/') - file) < (int)strlen(file);
    if (has_slash) {
        sys_execve(file, argv, envp_global);
        return;
    }

    path = NULL;
    if (envp_global) {
        for (char **ep = envp_global; *ep; ep++) {
            if ((*ep)[0] == 'P' && (*ep)[1] == 'A' &&
                (*ep)[2] == 'T' && (*ep)[3] == 'H' &&
                (*ep)[4] == '=') {
                path = *ep + 5;
                break;
            }
        }
    }
    if (!path) path = "/usr/local/bin:/usr/bin:/bin";

    p = path;
    while (*p) {
        const char *next = strchrnul(p, ':');
        int len = (int)(next - p);
        int i;
        for (i = 0; i < len && i < (int)sizeof(buf) - 2; i++)
            buf[i] = p[i];
        buf[i] = '/';
        i++;
        for (int j = 0; file[j] && i < (int)sizeof(buf) - 1; j++, i++)
            buf[i] = file[j];
        buf[i] = '\0';
        sys_execve(buf, argv, envp_global);
        p = *next ? next + 1 : next;
    }
}

void spawn(const Arg *arg)
{
    const char **argv = (const char **)arg->v;
    if (!argv || !argv[0])
        return;
    if (sys_fork() == 0) {
        if (sys_fork() == 0) {
            if (dpy)
                sys_close(ConnectionNumber(dpy));
            sys_setsid();
            try_exec(argv[0], (char *const *)argv);
            sys_write(2, "fiwm: execvp failed\n", 20);
            sys_exit(1);
        }
        sys_exit(0);
    }
    sys_wait4(-1, NULL, 0, NULL);
}
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

Node *node_insert(Monitor *m, Client *c)
{
    Workspace *ws = m->ws;
    Node *focus = ws->focus;
    Node *newnode, *newleaf;

    newleaf = node_new();
    newleaf->client = c;

    if (!focus || !ws->root) {
        ws->root = newleaf;
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

                int sub_pos = -1;
                Node *q = n;
                while (q != p->parent) {
                    if (q->parent && q->parent->split != orient)
                        sub_pos = (q->parent->first == q) ? 0 : 1;
                    q = q->parent;
                }

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

static void
arrange_floating(Monitor *m)
{
    Client *c;
    Node *n;

    if (!m) return;

    for (c = clients; c; c = c->next) {
        if (!c->is_floating) continue;
        n = node_find_client(m->ws->root, c);
        if (!n) continue;
        XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
        XSetWindowBorderWidth(dpy, c->win, borderpx);
        XSetWindowBorder(dpy, c->win,
                         (n == m->ws->focus) ? border_active : border_inactive);
    }
}
void arrange_node(Node *n, int x, int y, int w, int h, int gap)
{
    Client *c;
    if (!n) return;

    if (n->is_leaf) {
        c = n->client;
        if (!c || c->is_fullscreen || c->is_floating)
            return;
        XMoveResizeWindow(dpy, c->win, x + borderpx, y + borderpx,
                          MAX(1, w - 2 * borderpx), MAX(1, h - 2 * borderpx));
        return;
    }

    if (n->split == SPLIT_VERTICAL) {
        int fw = MAX(1, (int)(w * default_ratio) - (gap > 0 ? gap/2 : 0));
        arrange_node(n->first,  x, y, fw, h, gap);
        arrange_node(n->second, x + fw + gap, y, MAX(1, w - fw - gap), h, gap);
    } else {
        int fh = MAX(1, (int)(h * default_ratio) - (gap > 0 ? gap/2 : 0));
        arrange_node(n->first,  x, y, w, fh, gap);
        arrange_node(n->second, x, y + fh + gap, w, MAX(1, h - fh - gap), gap);
    }
}

void arrange(Monitor *m)
{
    int ogap, igap;
    int wx, wy, ww, wh;

    if (!m) return;

    ogap = (gappx > 0) ? gappx : 0;
    igap = (gappx > 0) ? gappx : 0;

    if (m->barwin && !(m->ws->focus && m->ws->focus->client &&
        m->ws->focus->client->is_fullscreen))
        XMoveResizeWindow(dpy, m->barwin,
                          m->x + ogap, m->y + ogap,
                          m->w - 2 * ogap, BAR_HEIGHT);

    wx = m->x + ogap;
    wy = m->y + ogap + BAR_HEIGHT + ogap;
    ww = m->w - 2 * ogap;
    wh = m->h - BAR_HEIGHT - 3 * ogap;

    if (m->ws->root)
        arrange_node(m->ws->root, wx, wy, ww, wh, igap);

    arrange_floating(m);

    if (m->ws->focus && m->ws->focus->client) {
        Client *c = m->ws->focus->client;
        if (c->is_fullscreen)
            XMoveResizeWindow(dpy, c->win, m->x, m->y, m->w, m->h);
        XRaiseWindow(dpy, c->win);
    }

    drawbar(m);
    XFlush(dpy);
}
void manage(Window w, XWindowAttributes *wa)
{
    Client *c;
    XSetWindowAttributes swa;
    Atom actual, *atoms;
    int format;
    unsigned long n, left;
    unsigned char *data;

    if (client_freelist) {
        c = client_freelist;
        client_freelist = c->next;
        memset(c, 0, sizeof(Client));
    } else {
        c = ecalloc(1, sizeof(Client));
    }
    c->win = w;
    c->x   = wa->x;
    c->y   = wa->y;
    c->w   = wa->width;
    c->h   = wa->height;

    if (XGetWindowProperty(dpy, w, netatom[NET_WM_WINDOW_TYPE], 0L, 2L,
                           False, XA_ATOM, &actual, &format,
                           &n, &left, &data) == Success && data) {
        atoms = (Atom *)data;
        for (unsigned long i = 0; i < n; i++)
            if (atoms[i] == netatom[NET_WM_WINDOW_TYPE_DIALOG])
                c->is_dialog = c->is_floating = 1;
        XFree(data);
    }

    if (clients) clients->prev = c;
    c->next = clients;
    c->prev = NULL;
    clients = c;

    swa.event_mask = EnterWindowMask | FocusChangeMask | PropertyChangeMask |
                     ButtonPressMask | SubstructureNotifyMask;
    XSetWindowBorderWidth(dpy, w, borderpx);
    XSetWindowBorder(dpy, w, border_inactive);
    XChangeWindowAttributes(dpy, w, CWEventMask, &swa);
    mask_children(w);

    XGrabButton(dpy, Button1, MODKEY, w, False, ButtonPressMask,
                GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(dpy, Button3, MODKEY, w, False, ButtonPressMask,
                GrabModeAsync, GrabModeAsync, None, None);

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

    for (m = mons; m; m = m->next) {
        n = node_find_client(m->ws->root, c);
        if (!n) continue;
        newfocus = node_detach(n, m->ws);
        if (newfocus)
            m->ws->focus = newfocus;
        else
            m->ws->focus = NULL;
        node_free(n);
        arrange(m);
        break;
    }

    if (c->next) c->next->prev = c->prev;
    if (c->prev) c->prev->next = c->next;
    else         clients = c->next;

    XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
    c->next = client_freelist;
    c->prev = NULL;
    client_freelist = c;
}

void focus(Monitor *m, Node *n)
{
    if (!m || !n || !n->client) return;
    if (m->ws->focus && m->ws->focus->client && m->ws->focus != n)
        XSetWindowBorder(dpy, m->ws->focus->client->win, border_inactive);
    m->ws->focus = n;
    XSetWindowBorder(dpy, n->client->win, border_active);
    XSetInputFocus(dpy, n->client->win, RevertToPointerRoot, CurrentTime);
    XRaiseWindow(dpy, n->client->win);
    XChangeProperty(dpy, root, netatom[NET_ACTIVE_WINDOW],
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)&n->client->win, 1);
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
    Client *c;
    int di, ox, oy;
    unsigned int du;
    Window dw;
    XEvent ev;

    SELCLIENT(c);
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
    Client *c;
    int di, mx, my;
    unsigned int du;
    Window dw;
    XEvent ev;

    SELCLIENT(c);
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
#ifdef XINERAMA
int isuniquegeom(XineramaScreenInfo *unique, int n, XineramaScreenInfo *info)
{
    for (int i = 0; i < n; i++)
        if (unique[i].x_org  == info->x_org  &&
            unique[i].y_org  == info->y_org  &&
            unique[i].width  == info->width  &&
            unique[i].height == info->height)
            return 0;
    return 1;
}
#endif

static Monitor *
monitor_new(int num, int x, int y, int w, int h)
{
    Monitor *m = ecalloc(1, sizeof(Monitor));
    m->num = num;
    m->x = x; m->y = y; m->w = w; m->h = h;
    m->curtag = 0;
    m->ws = &workspaces[0];
    m->next_split = -1;
    return m;
}

void updategeom(void)
{
    Monitor *m;

    while (mons) {
        m = mons->next;
        free(mons);
        mons = m;
    }
    mons = NULL;

#ifdef XINERAMA
    if (XineramaIsActive(dpy)) {
        int n;
        XineramaScreenInfo *info = XineramaQueryScreens(dpy, &n);
        XineramaScreenInfo *unique = ecalloc(n, sizeof(XineramaScreenInfo));
        int nunique = 0;

        for (int i = 0; i < n; i++)
            if (isuniquegeom(unique, nunique, &info[i]))
                unique[nunique++] = info[i];

        for (int i = 0; i < nunique; i++) {
            m = monitor_new(i, unique[i].x_org, unique[i].y_org,
                            unique[i].width, unique[i].height);
            m->next = mons;
            mons = m;
        }
        XFree(info);
        free(unique);
    } else
#endif
    {
        mons = monitor_new(0, 0, 0,
                           DisplayWidth(dpy, XDefaultScreen(dpy)),
                           DisplayHeight(dpy, XDefaultScreen(dpy)));
    }

    selmon = mons;
}

static void
update_bar_visibility(Monitor *m)
{
    if (!m || !m->barwin) return;
    if (m->ws->focus && m->ws->focus->client &&
        m->ws->focus->client->is_fullscreen)
        XUnmapWindow(dpy, m->barwin);
    else
        XMapWindow(dpy, m->barwin);
}

static Monitor *
monitor_in_direction(Monitor *m, int orient, int dir)
{
    Monitor *target = NULL, *tm;

    for (tm = mons; tm; tm = tm->next) {
        if (tm == m) continue;
        if (orient == SPLIT_VERTICAL) {
            if (dir == 0 && tm->x + tm->w <= m->x &&
                tm->y < m->y + m->h && tm->y + tm->h > m->y) {
                if (!target || tm->x + tm->w > target->x + target->w)
                    target = tm;
            } else if (dir == 1 && tm->x >= m->x + m->w &&
                tm->y < m->y + m->h && tm->y + tm->h > m->y) {
                if (!target || tm->x < target->x)
                    target = tm;
            }
        } else {
            if (dir == 0 && tm->y + tm->h <= m->y &&
                tm->x < m->x + m->w && tm->x + tm->w > m->x) {
                if (!target || tm->y + tm->h > target->y + target->h)
                    target = tm;
            } else if (dir == 1 && tm->y >= m->y + m->h &&
                tm->x < m->x + m->w && tm->x + tm->w > m->x) {
                if (!target || tm->y < target->y)
                    target = tm;
            }
        }
    }
    return target;
}

void focusworkspace(const Arg *arg)
{
    Monitor *m, *om, *prev;
    Workspace *oldws;
    int tag = arg->i;

    if (tag < 0 || tag >= WORKSPACE_COUNT || !selmon)
        return;

    prev = selmon;
    m = selmon;

    /* Redirect to pinned monitor if this workspace has a home */
    if (tagmap[tag] >= 0) {
        for (Monitor *tm = mons; tm; tm = tm->next) {
            if (tm->num == tagmap[tag] && tm != m) {
                m = tm;
                selmon = tm;
                break;
            }
        }
    }

    /* Already active on this monitor */
    if (m->curtag == tag) {
        if (m->ws->focus && m->ws->focus->client)
            focus(m, m->ws->focus);
        if (prev != m) { prev->dirty = 1; m->dirty = 1; }
        update_bar_visibility(m);
        arrange(m);
        if (prev != m) drawbar(prev);
        return;
    }

    oldws = m->ws;
    node_hide_foreach(oldws->root);

    om = NULL;
    for (om = mons; om; om = om->next) {
        if (om != m && om->curtag == tag) {
            Workspace *tmpws = m->ws;
            int tmptag = m->curtag;
            m->curtag = tag;
            m->ws     = &workspaces[tag];
            om->curtag = tmptag;
            om->ws     = tmpws;
            om->dirty = 1;
            break;
        }
    }
    if (!om) {
        m->curtag = tag;
        m->ws     = &workspaces[tag];
    } else {
        update_bar_visibility(om);
    }
    m->dirty = 1;

    if (m->ws->focus && m->ws->focus->client)
        focus(m, m->ws->focus);
    else if (m->ws->root) {
        Node *first = node_first_leaf(m->ws->root);
        if (first)
            focus(m, first);
    }

    if (prev != m) prev->dirty = 1;
    update_bar_visibility(m);
    arrange(m);
    if (prev != m) drawbar(prev);
}

void focusworkspace_next(const Arg *arg)
{
    if (!selmon) return;
    int tag = (selmon->curtag + 1) % WORKSPACE_COUNT;
    Arg a = {.i = tag};
    focusworkspace(&a);
}

void focusworkspace_prev(const Arg *arg)
{
    if (!selmon) return;
    int tag = (selmon->curtag - 1 + WORKSPACE_COUNT) % WORKSPACE_COUNT;
    Arg a = {.i = tag};
    focusworkspace(&a);
}
void drawbar(Monitor *m)
{
    char buf[8];
    int i, x, sw, ntags, idx;
    Drawable d;
    int tags[WORKSPACE_COUNT];

    if (!m || !m->barwin) return;
    if (!m->dirty) return;
    m->dirty = 0;

    ntags = 0;
    for (i = 0; i < WORKSPACE_COUNT; i++)
        if (tagmap[i] == m->num)
            tags[ntags++] = i;

    if (ntags == 0) return;

    {
        XWindowAttributes wa;
        sw = (XGetWindowAttributes(dpy, m->barwin, &wa))
             ? wa.width / ntags : 1;
    }

    if (!bar_gc)
        bar_gc = XCreateGC(dpy, m->barwin, 0, NULL);

    d = m->barpm ? m->barpm : m->barwin;

    XSetForeground(dpy, bar_gc, bar_bg);
    XFillRectangle(dpy, d, bar_gc, 0, 0, sw * ntags, BAR_HEIGHT);

    for (idx = 0; idx < ntags; idx++) {
        i = tags[idx];
        x = idx * sw;
        itoa(i + 1, buf, sizeof(buf));
        XSetForeground(dpy, bar_gc, (m == selmon && m->curtag == i) ? bar_fg : bar_hl);
        XDrawString(dpy, d, bar_gc, x + sw/2 - 4,
                    (BAR_HEIGHT / 2) + 5, buf, strlen(buf));
    }

    if (m->barpm)
        XCopyArea(dpy, m->barpm, m->barwin, bar_gc, 0, 0,
                  sw * ntags, BAR_HEIGHT, 0, 0);
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
                                  CopyFromParent, InputOutput,
                                  CopyFromParent,
                                  CWOverrideRedirect | CWEventMask,
                                  &wa);
        m->barpm = XCreatePixmap(dpy, root, m->w, BAR_HEIGHT,
                                 DefaultDepth(dpy, XDefaultScreen(dpy)));

        XMapWindow(dpy, m->barwin);
        XLowerWindow(dpy, m->barwin);
    }
}
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

    for (m = mons; m; m = m->next) {
        if (e->xbutton.window == m->barwin) {
            XWindowAttributes wa;
            if (XGetWindowAttributes(dpy, m->barwin, &wa)) {
                int i, ntags, tags[WORKSPACE_COUNT];
                ntags = 0;
                for (i = 0; i < WORKSPACE_COUNT; i++)
                    if (tagmap[i] == m->num)
                        tags[ntags++] = i;
                if (ntags > 0) {
                    int sw = wa.width / ntags;
                    int clicked = e->xbutton.x / sw;
                    if (clicked >= 0 && clicked < ntags) {
                        Arg a = {.i = tags[clicked]};
                        focusworkspace(&a);
                    }
                }
            }
            return;
        }
    }

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

    if (!c) return;
    for (m = mons; m; m = m->next) {
        n = node_find_client(m->ws->root, c);
        if (n) { focus(m, n); arrange(m); return; }
    }
}

static void mask_children(Window w)
{
    Window r, par, *kids;
    unsigned int nk;
    if (!XQueryTree(dpy, w, &r, &par, &kids, &nk)) return;
    for (unsigned int i = 0; i < nk; i++)
        XSelectInput(dpy, kids[i], EnterWindowMask);
    XFree(kids);
}

void enternotify(XEvent *e)
{
    XCrossingEvent *ev = &e->xcrossing;
    Monitor *m, *prev;
    Client *c;
    Node *n;
    Window par, *kids, w;
    unsigned int nk;

    /* Walk up to find a managed client under the pointer */
    c = find_client(ev->window);
    w = ev->window;
    while (!c && w != root && w != None) {
        if (!XQueryTree(dpy, w, &(Window){0}, &par, &kids, &nk)) break;
        if (kids) XFree(kids);
        if (par == root || par == None) break;
        c = find_client(par);
        w = par;
    }
    if (!c) return;  /* not entering a managed client */

    /* Find monitor by client, not by coordinates */
    n = NULL;
    for (m = mons; m; m = m->next) {
        n = node_find_client(m->ws->root, c);
        if (n) break;
    }
    if (!m || !n) return;

    prev = selmon;
    if (m != prev) {
        selmon = m;
        if (prev) { prev->dirty = 1; drawbar(prev); }
        m->dirty = 1;
    }
    if (n != m->ws->focus)
        focus(m, n);
    drawbar(m);
}

void createnotify(XEvent *e)
{
    XCreateWindowEvent *ev = &e->xcreatewindow;
    XSelectInput(dpy, ev->window, EnterWindowMask);
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
int xerror(Display *dpy_, XErrorEvent *ee)
{
    (void)dpy_;
    if (ee->error_code == BadWindow ||
        ee->error_code == BadDrawable ||
        ee->error_code == BadColor)
        return 0;
    return 0;
}
void quit(const Arg *arg)
{
    running = 0;
}

void killclient(const Arg *arg)
{
    Client *c;
    XEvent ev;

    SELCLIENT(c);

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
    SELCLIENT(c);

    c->is_fullscreen = !c->is_fullscreen;
    if (c->is_fullscreen) {
        XSetWindowBorderWidth(dpy, c->win, 0);
        XMoveResizeWindow(dpy, c->win, selmon->x, selmon->y,
                          selmon->w, selmon->h);
        XUnmapWindow(dpy, selmon->barwin);
        XRaiseWindow(dpy, c->win);
    } else {
        XMapWindow(dpy, selmon->barwin);
        XSetWindowBorderWidth(dpy, c->win, borderpx);
        XSetWindowBorder(dpy, c->win,
                         (c == selmon->ws->focus->client) ? border_active : border_inactive);
        arrange(selmon);
    }
}

void togglefloating(const Arg *arg)
{
    Client *c;
    int fw, fh;
    SELCLIENT(c);

    c->is_floating = !c->is_floating;
    if (c->is_floating) {
        c->tile_x = c->x; c->tile_y = c->y;
        c->tile_w = c->w; c->tile_h = c->h;
        fw = selmon->w * 0.6f;
        fh = selmon->h * 0.6f;
        c->x = selmon->x + (selmon->w - fw) / 2;
        c->y = selmon->y + (selmon->h - fh) / 2;
        c->w = fw; c->h = fh;
        XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
        XSetWindowBorderWidth(dpy, c->win, borderpx);
        XSetWindowBorder(dpy, c->win, border_active);
        XRaiseWindow(dpy, c->win);
    } else {
        c->x = c->tile_x; c->y = c->tile_y;
        c->w = c->tile_w; c->h = c->tile_h;
    }
    arrange(selmon);
}

void rotatecmd(const Arg *arg)
{
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
        return;
    }

    {
        Monitor *om = monitor_in_direction(selmon, orient, dir);
        if (om) {
            Node *n = om->ws->focus;
            if ((!n || !n->client) && om->ws->root)
                n = node_first_leaf(om->ws->root);
            if (n && n->client) {
                Monitor *prev = selmon;
                selmon = om;
                om->dirty = 1;
                prev->dirty = 1;
                focus(om, n);
                arrange(om);
                drawbar(prev);
            }
        }
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

    if (raw >= 0 && raw < WORKSPACE_COUNT) {
        Client *c = n->client;
        int tag = raw;
        if (!c) return;

        Node *newfocus = node_detach(n, ws);
        ws->focus = newfocus;
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
        arrange(selmon);
        return;
    }

    raw -= 100;
    orient = raw >> 1;
    dir    = raw & 1;

    target = node_in_direction(n, orient, dir);
    if (target && target->is_leaf && target->client && n->client) {
        tmpc = n->client;
        n->client = target->client;
        target->client = tmpc;
        focus(selmon, target);
        arrange(selmon);
        return;
    }

    if (!n->client) return;
    {
        Monitor *om = monitor_in_direction(selmon, orient, dir);
        if (om) {
            Client *c = n->client;
            Workspace *tws = om->ws;

            Node *newfocus = node_detach(n, ws);
            ws->focus = newfocus;
            node_free(n);

            Node *nl = node_new();
            nl->client = c;

            if (!tws->root) {
                tws->root = nl;
                tws->focus = nl;
            } else {
                Node *ff = tws->focus ? tws->focus : node_first_leaf(tws->root);
                Node *nn = node_new();
                nn->is_leaf = 0;
                nn->split = orient;
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

            {
                Monitor *prev = selmon;
                selmon = om;
                om->dirty = 1;
                prev->dirty = 1;
                focus(om, nl);
                arrange(om);
                arrange(prev);
            }
        }
    }
}

void setlayoutcmd(const Arg *arg)
{
    if (selmon) selmon->next_split = arg->i;
}
void grabkeys(void)
{
    unsigned int i, j;
    KeySym keysym;
    unsigned int modifiers[] = { 0, LockMask, 0, LockMask };

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
                wa.override_redirect)
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
    root = RootWindow(dpy, XDefaultScreen(dpy));
    {
        Colormap cmap = DefaultColormap(dpy, XDefaultScreen(dpy));
        XColor c;
        XAllocNamedColor(dpy, cmap, colors[COL_BORDER_ACTIVE], &c, &c); border_active = c.pixel;
        XAllocNamedColor(dpy, cmap, colors[COL_BORDER_INACTIVE], &c, &c); border_inactive = c.pixel;
        XAllocNamedColor(dpy, cmap, colors[COL_BAR_BG], &c, &c); bar_bg = c.pixel;
        XAllocNamedColor(dpy, cmap, colors[COL_BAR_FG], &c, &c); bar_fg = c.pixel;
        XAllocNamedColor(dpy, cmap, colors[COL_BAR_HL], &c, &c); bar_hl = c.pixel;
    }

    for (i = 0; i < WORKSPACE_COUNT; i++) {
        workspaces[i].root  = NULL;
        workspaces[i].focus = NULL;
    }

    wmatom[WM_PROTOCOLS]              = XInternAtom(dpy, "WM_PROTOCOLS", False);
    wmatom[WM_DELETE_WINDOW]          = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    wmatom[WM_STATE]                  = XInternAtom(dpy, "WM_STATE", False);
    netatom[NET_WM_STATE]             = XInternAtom(dpy, "_NET_WM_STATE", False);
    netatom[NET_WM_STATE_FULLSCREEN]  = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
    netatom[NET_ACTIVE_WINDOW]        = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
    netatom[NET_WM_WINDOW_TYPE]       = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    netatom[NET_WM_WINDOW_TYPE_DIALOG]= XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);

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

    wa.cursor = XCreateFontCursor(dpy, XC_left_ptr);
    wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask |
                    EnterWindowMask | ButtonPressMask |
                    StructureNotifyMask | PropertyChangeMask;
    XChangeWindowAttributes(dpy, root, CWEventMask | CWCursor, &wa);

    xerrorxlib = XSetErrorHandler(xerror);

    updategeom();
    createbars();
    grabkeys();
    scan();

    selmon = mons;
    for (Monitor *m = mons; m; m = m->next)
        arrange(m);

    /* Launch autostart commands */
    for (size_t i = 0; i < LENGTH(autostart); i++) {
        Arg a = { .v = autostart[i] };
        spawn(&a);
    }
}

void run(void)
{
    XEvent ev;
    /* Drain stale EnterNotify from startup so selmon isn't overridden */
    while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
    while (running && !XNextEvent(dpy, &ev)) {
        switch (ev.type) {
        case KeyPress:         keypress(&ev);       break;
        case ButtonPress:      buttonpress(&ev);    break;
        case EnterNotify:      enternotify(&ev);    break;
        case CreateNotify:     createnotify(&ev);   break;
        case MapRequest:       maprequest(&ev);     break;
        case UnmapNotify:      unmapnotify(&ev);    break;
        case DestroyNotify:    destroywindow(&ev);  break;
        case ConfigureRequest: configurerequest(&ev); break;
        case Expose:
            {
                Monitor *m;
                for (m = mons; m; m = m->next)
                    if (m->barwin == ev.xexpose.window)
                        { m->dirty = 1; drawbar(m); }
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

    if (bar_gc) XFreeGC(dpy, bar_gc);

    node_pool_reset();

    m = mons;
    while (m) {
        if (m->barpm)
            XFreePixmap(dpy, m->barpm);
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

static void setup_environ(char **envp)
{
    envp_global = envp;
}

__attribute__((used, noinline))
int wm_main(int argc, char **argv, char **envp)
{

    setup_environ(envp);

    if (argc == 2 && strcmp(argv[1], "-v") == 0) {
        sys_write(1, "fiwm 0.1\n", 9);
        return 0;
    }

    setup();
    run();
    cleanup();
    return 0;
}

__attribute__((naked, noreturn))
void _start(void)
{
    __asm__ __volatile__(
        "xorl  %%ebp, %%ebp\n\t"
        "popq  %%rdi\n\t"
        "movq  %%rsp, %%rsi\n\t"
        "leaq  (%%rsi,%%rdi,8), %%rdx\n\t"
        "addq  $8, %%rdx\n\t"
        "andq  $-16, %%rsp\n\t"
        "call  wm_main\n\t"
        "movl  %%eax, %%edi\n\t"
        "movl  $60, %%eax\n\t"
        "syscall\n\t"
        :
        :
        : "rdi", "rsi", "rdx", "memory"
        );
    __builtin_unreachable();
}
