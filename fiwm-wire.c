/* fiwm-wire: Minimalist BSP Tiling Window Manager for X11
 * 100% Freestanding, Zero-libc, Raw X11 Wire Protocol over UNIX Socket
 * Single-file static build for x86_64
 */

/* Freestanding UNIX socket definitions */
struct sockaddr_un {
    unsigned short sun_family;
    char sun_path[108];
};
#define AF_UNIX      1
#define SOCK_STREAM  1



#ifndef NULL
#define NULL ((void *)0)
typedef unsigned long size_t;
#endif

#define SYS_READ      0
#define SYS_WRITE     1
#define SYS_CLOSE     3
#define SYS_POLL      7
#define SYS_SOCKET   41
#define SYS_CONNECT  42
#define SYS_EXIT     60

/* Types matching X11 API */
typedef unsigned long  Window;
typedef unsigned long  Drawable;
typedef unsigned long  Pixmap;
typedef unsigned long  Colormap;
typedef unsigned long  Atom;
typedef unsigned long  VisualID;
typedef unsigned long  Time;
typedef unsigned int   KeySym;
typedef unsigned char  KeyCode;
typedef struct _XGC   *GC;

typedef struct {
    int x, y;
    int width, height;
    int border_width;
    Window root;
    int depth;
    VisualID visual;
    int c_class;
    int bit_gravity;
    int win_gravity;
    int backing_store;
    unsigned long backing_planes;
    unsigned long backing_pixel;
    int save_under;
    Colormap colormap;
    int map_installed;
    int map_state;
    long all_event_masks;
    long your_event_mask;
    long do_not_propagate_mask;
    int override_redirect;
    void *screen;
} XWindowAttributes;

typedef struct {
    Pixmap background_pixmap;
    unsigned long background_pixel;
    Pixmap border_pixmap;
    unsigned long border_pixel;
    int bit_gravity;
    int win_gravity;
    int backing_store;
    unsigned long backing_planes;
    unsigned long backing_pixel;
    int save_under;
    long event_mask;
    long do_not_propagate_mask;
    int override_redirect;
    Colormap colormap;
    unsigned long cursor;
} XSetWindowAttributes;

typedef struct {
    int x, y;
    int width, height;
    int border_width;
    Window sibling;
    int stack_mode;
} XWindowChanges;

typedef struct {
    unsigned long pixel;
    unsigned short red, green, blue;
    char flags;
    char pad;
} XColor;

typedef struct {
    int max_keypermod;
    KeyCode *modifiermap;
} XModifierKeymap;

/* Event definitions */
#define KeyPress                2
#define KeyRelease              3
#define ButtonPress             4
#define ButtonRelease           5
#define MotionNotify            6
#define EnterNotify             7
#define LeaveNotify             8
#define FocusIn                 9
#define FocusOut                10
#define KeymapNotify            11
#define Expose                  12
#define GraphicsExpose          13
#define NoExpose                14
#define VisibilityNotify        15
#define CreateNotify            16
#define DestroyNotify           17
#define UnmapNotify             18
#define MapNotify               19
#define MapRequest              20
#define ReparentNotify          21
#define ConfigureNotify         22
#define ConfigureRequest        23
#define GravityNotify           24
#define ResizeRequest           25
#define CirculateNotify         26
#define CirculateRequest        27
#define PropertyNotify          28
#define SelectionClear          29
#define SelectionRequest        30
#define SelectionNotify         31
#define ColormapNotify          32
#define ClientMessage           33
#define MappingNotify           34

#define KeyPressMask             (1L<<0)
#define KeyReleaseMask           (1L<<1)
#define ButtonPressMask          (1L<<2)
#define ButtonReleaseMask        (1L<<3)
#define EnterWindowMask          (1L<<4)
#define LeaveWindowMask          (1L<<5)
#define PointerMotionMask        (1L<<6)
#define ExposureMask             (1L<<15)
#define StructureNotifyMask      (1L<<17)
#define SubstructureNotifyMask   (1L<<19)
#define SubstructureRedirectMask (1L<<20)
#define FocusChangeMask          (1L<<21)
#define PropertyChangeMask       (1L<<22)
#define NoEventMask              0L

#define CurrentTime              0L
#define AnyButton                0
#define AnyKey                   0
#define AnyModifier              (1<<15)
#define GrabModeAsync            1
#define None                     0L
#define True                     1
#define False                    0
#define Success                  0
#define PropModeReplace          0
#define RevertToPointerRoot      1
#define IsViewable               2

#define GrabSuccess              0
#define CopyFromParent           0L
#define InputOutput              1
#define CWOverrideRedirect       (1L<<9)

#define ShiftMask                (1<<0)
#define LockMask                 (1<<1)
#define ControlMask              (1<<2)
#define Mod1Mask                 (1<<3)
#define Mod2Mask                 (1<<4)
#define Mod3Mask                 (1<<5)
#define Mod4Mask                 (1<<6)
#define Mod5Mask                 (1<<7)

#define Button1                  1
#define Button2                  2
#define Button3                  3
#define Button4                  4
#define Button5                  5

#define CWBackPixel              (1L<<1)
#define CWEventMask              (1L<<11)
#define CWCursor                 (1L<<14)

#define XA_ATOM                  4UL
#define XA_CARDINAL              6UL
#define XA_WINDOW                33UL

#define XK_1                     0x0031
#define XK_2                     0x0032
#define XK_3                     0x0033
#define XK_4                     0x0034
#define XK_5                     0x0035
#define XK_6                     0x0036
#define XK_7                     0x0037
#define XK_8                     0x0038
#define XK_9                     0x0039
#define XK_Return                0xff0d
#define XK_space                 0x0020
#define XK_q                     0x0071
#define XK_w                     0x0077
#define XK_d                     0x0064
#define XK_c                     0x0063
#define XK_h                     0x0068
#define XK_j                     0x006a
#define XK_k                     0x006b
#define XK_l                     0x006c
#define XK_r                     0x0072
#define XK_b                     0x0062
#define XK_v                     0x0076
#define XK_a                     0x0061
#define XK_s                     0x0073
#define XK_f                     0x0066
#define XK_Num_Lock              0xff7f

#define XC_left_ptr              68

typedef struct {
    int type;
    unsigned long serial;
    int send_event;
    void *display;
    Window window;
    Window root;
    Window subwindow;
    Time time;
    int x, y;
    int x_root, y_root;
    unsigned int state;
    unsigned int keycode;
    int same_screen;
} XKeyEvent;

typedef struct {
    int type;
    unsigned long serial;
    int send_event;
    void *display;
    Window window;
    Window root;
    Window subwindow;
    Time time;
    int x, y;
    int x_root, y_root;
    unsigned int state;
    unsigned int button;
    int same_screen;
} XButtonEvent;

typedef struct {
    int type;
    unsigned long serial;
    int send_event;
    void *display;
    Window window;
    Window root;
    Window subwindow;
    Time time;
    int x, y;
    int x_root, y_root;
    unsigned int state;
    char is_hint;
    int same_screen;
} XMotionEvent;

typedef struct {
    int type;
    unsigned long serial;
    int send_event;
    void *display;
    Window event;
    Window window;
    int x, y;
    int width, height;
    int border_width;
    Window above;
    int override_redirect;
} XConfigureEvent;

typedef struct {
    int type;
    unsigned long serial;
    int send_event;
    void *display;
    Window parent;
    Window window;
    int x, y;
    int width, height;
    int border_width;
    Window above;
    int detail;
    unsigned long value_mask;
} XConfigureRequestEvent;

typedef struct {
    int type;
    unsigned long serial;
    int send_event;
    void *display;
    Window parent;
    Window window;
} XMapRequestEvent;

typedef struct {
    int type;
    unsigned long serial;
    int send_event;
    void *display;
    Window event;
    Window window;
} XDestroyWindowEvent;

typedef struct {
    int type;
    unsigned long serial;
    int send_event;
    void *display;
    Window window;
    int x, y;
    int width, height;
    int count;
} XExposeEvent;

typedef struct {
    int type;
    unsigned long serial;
    int send_event;
    void *display;
    Window window;
    Atom message_type;
    int format;
    union {
        char b[20];
        short s[10];
        long l[5];
    } data;
} XClientMessageEvent;

typedef struct {
    int type;
    unsigned long serial;
    int send_event;
    void *display;
    Window window;
    Atom atom;
    Time time;
    int state;
} XPropertyEvent;

typedef struct {
    int type;
    unsigned long serial;
    int send_event;
    void *display;
    Window window;
    Window root;
    Window subwindow;
    Time time;
    int x, y;
    int x_root, y_root;
    int mode;
    int detail;
    int same_screen;
    int focus;
    unsigned int state;
} XCrossingEvent;

typedef struct {
    int type;
    unsigned long serial;
    int send_event;
    void *display;
    Window parent;
    Window window;
    int x, y;
    int width, height;
    int border_width;
    int override_redirect;
} XCreateWindowEvent;

typedef struct {
    int type;
    unsigned long serial;
    int send_event;
    void *display;
    Window event;
    Window window;
    int from_configure;
} XUnmapEvent;

typedef union _XEvent {
    int type;
    XKeyEvent xkey;
    XButtonEvent xbutton;
    XMotionEvent xmotion;
    XCrossingEvent xcrossing;
    XCreateWindowEvent xcreatewindow;
    XUnmapEvent xunmap;
    XConfigureEvent xconfigure;
    XConfigureRequestEvent xconfigurerequest;
    XMapRequestEvent xmaprequest;
    XDestroyWindowEvent xdestroywindow;
    XExposeEvent xexpose;
    XClientMessageEvent xclient;
    XPropertyEvent xproperty;
    long pad[24];
} XEvent;

typedef struct {
    int type;
    void *display;
    unsigned long resourceid;
    unsigned long serial;
    unsigned char error_code;
    unsigned char request_code;
    unsigned char minor_code;
} XErrorEvent;

typedef struct {
    int fd;
    Window root;
    int width, height;
    unsigned int rid_base;
    unsigned int rid_mask;
    unsigned int next_rid;
} Display;

/* Syscall wrappers */
static inline long _xwire_sys3(long nr, long a1, long a2, long a3) {
    long ret;
    __asm__ __volatile__("syscall" : "=a"(ret) : "a"(nr), "D"(a1), "S"(a2), "d"(a3) : "rcx", "r11", "memory");
    return ret;
}

static inline int _xwire_write(int fd, const void *b, unsigned long n) {
    return (int)_xwire_sys3(SYS_WRITE, fd, (long)b, n);
}

static inline int _xwire_read(int fd, void *b, unsigned long n) {
    return (int)_xwire_sys3(SYS_READ, fd, (long)b, n);
}

static Display _global_display;

extern char **envp_global;

static inline Display *XOpenDisplay(const char *name) {
    int fd = (int)_xwire_sys3(SYS_SOCKET, AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return (void *)0;

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    char path[64] = "/tmp/.X11-unix/X0";

    const char *dstr = name;
    if (!dstr && envp_global) {
        for (char **ep = envp_global; *ep; ep++) {
            if ((*ep)[0] == 'D' && (*ep)[1] == 'I' && (*ep)[2] == 'S' &&
                (*ep)[3] == 'P' && (*ep)[4] == 'L' && (*ep)[5] == 'A' &&
                (*ep)[6] == 'Y' && (*ep)[7] == '=') {
                dstr = *ep + 8;
                break;
            }
        }
    }

    if (dstr && dstr[0] == ':') {
        int i = 0, j = 16;
        while (dstr[++i] && dstr[i] != '.' && j < 60)
            path[j++] = dstr[i];
        path[j] = '\0';
    }

    for (unsigned long k = 0; k < sizeof(path); k++) addr.sun_path[k] = path[k];

    if (_xwire_sys3(SYS_CONNECT, fd, (long)&addr, sizeof(addr)) < 0) {
        _xwire_sys3(SYS_CLOSE, fd, 0, 0);
        return (void *)0;
    }

    unsigned char setup_req[12] = { 'l', 0, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    _xwire_write(fd, setup_req, 12);

    unsigned char rep_hdr[8];
    if (_xwire_read(fd, rep_hdr, 8) != 8 || rep_hdr[0] != 1) {
        _xwire_sys3(SYS_CLOSE, fd, 0, 0);
        return (void *)0;
    }

    unsigned short add_len = *(unsigned short *)&rep_hdr[6];
    unsigned int total_body = (unsigned int)add_len * 4;
    unsigned char body[2048];
    unsigned int first_chunk = total_body > sizeof(body) ? sizeof(body) : total_body;
    unsigned int read_bytes = 0;
    while (read_bytes < first_chunk) {
        int r = _xwire_read(fd, body + read_bytes, first_chunk - read_bytes);
        if (r <= 0) break;
        read_bytes += (unsigned int)r;
    }

    /* Drain the rest of the setup reply data so socket is 100% clean */
    while (read_bytes < total_body) {
        char discard[512];
        unsigned int want = (total_body - read_bytes) > sizeof(discard) ? sizeof(discard) : (total_body - read_bytes);
        int r = _xwire_read(fd, discard, want);
        if (r <= 0) break;
        read_bytes += (unsigned int)r;
    }

    _global_display.fd = fd;
    _global_display.rid_base = *(unsigned int *)&body[4];
    _global_display.rid_mask = *(unsigned int *)&body[8];
    _global_display.next_rid = _global_display.rid_base;

    unsigned short vendor_len = *(unsigned short *)&body[16];
    unsigned char num_formats = body[21];
    int screen_offset = 32 + ((vendor_len + 3) & ~3) + (num_formats * 8);

    if (screen_offset + 40 <= (int)first_chunk) {
        _global_display.root   = *(unsigned int *)&body[screen_offset];
        _global_display.width  = *(unsigned short *)&body[screen_offset + 20];
        _global_display.height = *(unsigned short *)&body[screen_offset + 22];
    } else {
        _global_display.root   = 0x100;
        _global_display.width  = 1024;
        _global_display.height = 768;
    }

    return &_global_display;
}

static inline void XCloseDisplay(Display *d) {
    if (d && d->fd >= 0) {
        _xwire_sys3(SYS_CLOSE, d->fd, 0, 0);
        d->fd = -1;
    }
}

static inline int ConnectionNumber(Display *d) { return d ? d->fd : -1; }
static inline int XDefaultScreen(Display *d) { (void)d; return 0; }
static inline int DisplayWidth(Display *d, int s) { (void)s; return d ? d->width : 1024; }
static inline int DisplayHeight(Display *d, int s) { (void)s; return d ? d->height : 768; }
static inline Window XRootWindow(Display *d, int s) { (void)s; return d ? d->root : 0; }
static inline Window RootWindow(Display *d, int s) { (void)s; return d ? d->root : 0; }
static inline Colormap DefaultColormap(Display *d, int s) { (void)d; (void)s; return 0; }
static inline int DefaultDepth(Display *d, int s) { (void)d; (void)s; return 24; }

static inline Window XCreateWindow(Display *d, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width, int depth, unsigned int class_type, VisualID visual, unsigned long valmask, XSetWindowAttributes *a) {
    (void)class_type; (void)visual;
    unsigned int wid = d->next_rid++;
    /* Opcode 1: CreateWindow */
    unsigned int p[32];
    p[1] = wid;
    p[2] = (unsigned int)parent;
    p[3] = ((int)y << 16) | ((int)x & 0xFFFF);
    p[4] = (height << 16) | (width & 0xFFFF);
    p[5] = (1 << 16) | (border_width & 0xFFFF); /* class = InputOutput (1) in high 16, border_width in low 16 */
    p[6] = 0; /* visual = CopyFromParent */
    p[7] = (unsigned int)valmask;
    int idx = 8;
    if (valmask & (1 << 1)) p[idx++] = (unsigned int)a->background_pixel;
    if (valmask & (1 << 3)) p[idx++] = (unsigned int)a->border_pixel;
    if (valmask & (1 << 9)) p[idx++] = (unsigned int)a->override_redirect;
    if (valmask & (1 << 11)) p[idx++] = (unsigned int)a->event_mask;
    p[0] = (idx << 16) | (depth << 8) | 1;
    _xwire_write(d->fd, p, idx * 4);
    return wid;
}

static inline void XLowerWindow(Display *d, Window w) {
    /* Opcode 12: ConfigureWindow (CWStackMode = 64, Below = 1) */
    unsigned int p[4] = { (4 << 16) | 12, (unsigned int)w, 64, 1 };
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XConfigureWindow(Display *d, Window w, unsigned long valmask, XWindowChanges *wc) {
    unsigned int p[16];
    int idx = 3;
    p[1] = (unsigned int)w;
    p[2] = (unsigned int)valmask;
    if (valmask & 1) p[idx++] = (unsigned int)wc->x;
    if (valmask & 2) p[idx++] = (unsigned int)wc->y;
    if (valmask & 4) p[idx++] = (unsigned int)wc->width;
    if (valmask & 8) p[idx++] = (unsigned int)wc->height;
    if (valmask & 16) p[idx++] = (unsigned int)wc->border_width;
    if (valmask & 32) p[idx++] = (unsigned int)wc->sibling;
    if (valmask & 64) p[idx++] = (unsigned int)wc->stack_mode;
    p[0] = (idx << 16) | 12;
    _xwire_write(d->fd, p, idx * 4);
}

static inline void XSelectInput(Display *d, Window w, long mask) {
    unsigned int p[4];
    p[0] = (4 << 16) | 2;
    p[1] = (unsigned int)w;
    p[2] = 0x800;
    p[3] = (unsigned int)mask;
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XChangeWindowAttributes(Display *d, Window w, unsigned long valmask, XSetWindowAttributes *a) {
    unsigned int p[16];
    int idx = 3;
    unsigned long mask = valmask;
    if ((mask & (1 << 14)) && a->cursor == 0)
        mask &= ~(1 << 14);

    p[1] = (unsigned int)w;
    p[2] = (unsigned int)mask;
    if (mask & (1 << 1)) p[idx++] = (unsigned int)a->background_pixel;
    if (mask & (1 << 3)) p[idx++] = (unsigned int)a->border_pixel;
    if (mask & (1 << 11)) p[idx++] = (unsigned int)a->event_mask;
    if (mask & (1 << 14)) p[idx++] = (unsigned int)a->cursor;
    p[0] = (idx << 16) | 2;
    _xwire_write(d->fd, p, idx * 4);
}

static inline void XSetWindowBorderWidth(Display *d, Window w, unsigned int bw) {
    unsigned int p[4];
    p[0] = (4 << 16) | 12;
    p[1] = (unsigned int)w;
    p[2] = 16;
    p[3] = bw;
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XSetWindowBorder(Display *d, Window w, unsigned long pixel) {
    unsigned int p[4];
    p[0] = (4 << 16) | 2;
    p[1] = (unsigned int)w;
    p[2] = 8;
    p[3] = (unsigned int)pixel;
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XMoveResizeWindow(Display *d, Window w, int x, int y, unsigned int width, unsigned int height) {
    unsigned int p[7];
    p[0] = (7 << 16) | 12;
    p[1] = (unsigned int)w;
    p[2] = 15;
    p[3] = (unsigned int)x;
    p[4] = (unsigned int)y;
    p[5] = width;
    p[6] = height;
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XMoveWindow(Display *d, Window w, int x, int y) {
    unsigned int p[5];
    p[0] = (5 << 16) | 12;
    p[1] = (unsigned int)w;
    p[2] = 3;
    p[3] = (unsigned int)x;
    p[4] = (unsigned int)y;
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XResizeWindow(Display *d, Window w, unsigned int width, unsigned int height) {
    unsigned int p[5];
    p[0] = (5 << 16) | 12;
    p[1] = (unsigned int)w;
    p[2] = 12;
    p[3] = width;
    p[4] = height;
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XMapWindow(Display *d, Window w) {
    unsigned int p[2] = { (2 << 16) | 8, (unsigned int)w };
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XUnmapWindow(Display *d, Window w) {
    unsigned int p[2] = { (2 << 16) | 10, (unsigned int)w };
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XRaiseWindow(Display *d, Window w) {
    unsigned int p[4] = { (4 << 16) | 12, (unsigned int)w, 64, 0 };
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XSetInputFocus(Display *d, Window w, int revert, Time t) {
    (void)revert; (void)t;
    unsigned int p[3] = { (3 << 16) | (1 << 8) | 42, (unsigned int)w, 0 };
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XGrabKey(Display *d, int keycode, unsigned int mod, Window w, int owner, int sync_p, int sync_k) {
    (void)owner; (void)sync_p; (void)sync_k;
    unsigned int p[4];
    p[0] = (4 << 16) | (1 << 8) | 33; /* X_GrabKey: reqType=33, ownerEvents=1, length=4 */
    p[1] = (unsigned int)w;
    p[2] = (1 << 24) | (((unsigned int)(keycode & 0xFF)) << 16) | (mod & 0xFFFF); /* modifiers (0..15), key (16..23), pointerMode=1 (24..31) */
    p[3] = 1; /* keyboardMode=1 (0..7), pad (8..31) */
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XUngrabKey(Display *d, int keycode, unsigned int mod, Window w) {
    unsigned int p[3];
    p[0] = (3 << 16) | ((keycode & 0xFF) << 8) | 34; /* X_UngrabKey: reqType=34, key=keycode, length=3 */
    p[1] = (unsigned int)w;
    p[2] = mod & 0xFFFF;
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XGrabButton(Display *d, unsigned int btn, unsigned int mod, Window w, int owner, unsigned int mask, int sync_p, int sync_k, Window confine, unsigned long cursor) {
    (void)owner; (void)sync_p; (void)sync_k; (void)confine; (void)cursor;
    unsigned int p[6];
    p[0] = (6 << 16) | 28; /* X_GrabButton: reqType=28, ownerEvents=0, length=6 */
    p[1] = (unsigned int)w;
    p[2] = (1 << 24) | (1 << 16) | (mask & 0xFFFF); /* eventMask (0..15), pointerMode=1 (16..23), keyboardMode=1 (24..31) */
    p[3] = (unsigned int)confine;
    p[4] = (unsigned int)cursor;
    p[5] = ((mod & 0xFFFF) << 16) | (btn & 0xFF); /* button (0..7), pad (8..15), modifiers (16..31) */
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XUngrabButton(Display *d, unsigned int btn, unsigned int mod, Window w) {
    unsigned int p[3] = { (3 << 16) | (btn << 8) | 29, (unsigned int)w, mod };
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XFlush(Display *d) { (void)d; }
static inline void XSync(Display *d, int discard) { (void)d; (void)discard; }

struct pollfd {
    int fd;
    short events;
    short revents;
};

static inline int XNextEvent(Display *d, XEvent *e) {
    unsigned char buf[32];
    for (;;) {
        struct pollfd pfd;
        pfd.fd = d->fd;
        pfd.events = 1; /* POLLIN */
        pfd.revents = 0;
        int pr = (int)_xwire_sys3(SYS_POLL, (long)&pfd, 1, -1);
        if (pr <= 0) { e->type = 0; return 1; }

        int total_n = 0;
        while (total_n < 32) {
            int n = _xwire_read(d->fd, buf + total_n, 32 - total_n);
            if (n <= 0) { e->type = 0; return 1; }
            total_n += n;
        }

        unsigned char code = buf[0] & 0x7F;
        if (code == 20) {
            e->type = MapRequest;
            e->xmaprequest.window = *(unsigned int *)&buf[8];
            e->xmaprequest.parent = *(unsigned int *)&buf[4];
            return 0;
        } else if (code == 23) {
            e->type = ConfigureRequest;
            e->xconfigurerequest.window = *(unsigned int *)&buf[8];
            e->xconfigurerequest.x = *(short *)&buf[16];
            e->xconfigurerequest.y = *(short *)&buf[18];
            e->xconfigurerequest.width = *(unsigned short *)&buf[20];
            e->xconfigurerequest.height = *(unsigned short *)&buf[22];
            e->xconfigurerequest.border_width = *(unsigned short *)&buf[24];
            e->xconfigurerequest.value_mask = *(unsigned short *)&buf[28];
            return 0;
        } else if (code == 18) {
            e->type = UnmapNotify;
            e->xunmap.window = *(unsigned int *)&buf[8];
            e->xunmap.send_event = 0;
            return 0;
        } else if (code == 17) {
            e->type = DestroyNotify;
            e->xdestroywindow.window = *(unsigned int *)&buf[8];
            return 0;
        } else if (code == 16) {
            e->type = CreateNotify;
            e->xcreatewindow.window = *(unsigned int *)&buf[8];
            e->xcreatewindow.parent = *(unsigned int *)&buf[4];
            return 0;
        } else if (code == 12) {
            e->type = Expose;
            e->xexpose.window = *(unsigned int *)&buf[4];
            return 0;
        } else if (code == 7) {
            e->type = EnterNotify;
            e->xcrossing.window = *(unsigned int *)&buf[12];
            e->xcrossing.root = *(unsigned int *)&buf[8];
            e->xcrossing.subwindow = *(unsigned int *)&buf[16];
            e->xcrossing.x_root = *(short *)&buf[20];
            e->xcrossing.y_root = *(short *)&buf[22];
            e->xcrossing.x = *(short *)&buf[24];
            e->xcrossing.y = *(short *)&buf[26];
            e->xcrossing.mode = 0;
            e->xcrossing.detail = 0;
            return 0;
        } else if (code == 6) {
            e->type = MotionNotify;
            e->xmotion.window = *(unsigned int *)&buf[12];
            e->xmotion.root = *(unsigned int *)&buf[8];
            e->xmotion.subwindow = *(unsigned int *)&buf[16];
            e->xmotion.time = *(unsigned int *)&buf[4];
            e->xmotion.x_root = *(short *)&buf[20];
            e->xmotion.y_root = *(short *)&buf[22];
            e->xmotion.x = *(short *)&buf[24];
            e->xmotion.y = *(short *)&buf[26];
            e->xmotion.state = *(unsigned short *)&buf[28];
            return 0;
        } else if (code == 5) {
            e->type = ButtonRelease;
            e->xbutton.window = *(unsigned int *)&buf[12];
            e->xbutton.root = *(unsigned int *)&buf[8];
            e->xbutton.subwindow = *(unsigned int *)&buf[16];
            e->xbutton.time = *(unsigned int *)&buf[4];
            e->xbutton.x_root = *(short *)&buf[20];
            e->xbutton.y_root = *(short *)&buf[22];
            e->xbutton.x = *(short *)&buf[24];
            e->xbutton.y = *(short *)&buf[26];
            e->xbutton.button = buf[1];
            e->xbutton.state = *(unsigned short *)&buf[28];
            return 0;
        } else if (code == 4) {
            e->type = ButtonPress;
            e->xbutton.window = *(unsigned int *)&buf[12];
            e->xbutton.root = *(unsigned int *)&buf[8];
            e->xbutton.subwindow = *(unsigned int *)&buf[16];
            e->xbutton.time = *(unsigned int *)&buf[4];
            e->xbutton.x_root = *(short *)&buf[20];
            e->xbutton.y_root = *(short *)&buf[22];
            e->xbutton.x = *(short *)&buf[24];
            e->xbutton.y = *(short *)&buf[26];
            e->xbutton.button = buf[1];
            e->xbutton.state = *(unsigned short *)&buf[28];
            return 0;
        } else if (code == 2) {
            e->type = KeyPress;
            e->xkey.window = *(unsigned int *)&buf[12];
            e->xkey.keycode = buf[1];
            e->xkey.state = *(unsigned short *)&buf[28];
            return 0;
        } else {
            e->type = code;
            return 0;
        }
    }
}

static inline int XCheckMaskEvent(Display *d, long mask, XEvent *e) {
    (void)d; (void)mask; (void)e;
    return 0;
}

static inline unsigned long XCreateFontCursor(Display *d, unsigned int shape) { (void)d; (void)shape; return 0; }
static inline void XFree(void *p) { (void)p; }
static inline int (*XSetErrorHandler(int (*handler)(Display *, XErrorEvent *)))(Display *, XErrorEvent *) { return handler; }

static inline Atom XInternAtom(Display *d, const char *name, int only_if_exists) {
    if (!d || !name) return 0;
    int nbytes = 0;
    while (name[nbytes]) nbytes++;

    int pad = (4 - (nbytes & 3)) & 3;
    int total_bytes = 8 + nbytes + pad;
    int words = total_bytes >> 2;

    unsigned int p[64];
    p[0] = (words << 16) | ((only_if_exists ? 1 : 0) << 8) | 16; /* X_InternAtom */
    p[1] = (unsigned int)nbytes;

    unsigned char *bp = (unsigned char *)&p[2];
    for (int i = 0; i < nbytes; i++) bp[i] = name[i];
    for (int i = 0; i < pad; i++) bp[nbytes + i] = 0;

    _xwire_write(d->fd, p, total_bytes);

    unsigned char rep[32];
    int rep_read = 0;
    while (rep_read < 32) {
        int r = _xwire_read(d->fd, rep + rep_read, 32 - rep_read);
        if (r <= 0) break;
        rep_read += r;
    }
    if (rep_read == 32 && rep[0] == 1) {
        return (Atom)*(unsigned int *)&rep[8];
    }
    return 0;
}

static inline int XAllocNamedColor(Display *d, Colormap m, const char *name, XColor *scr, XColor *exact) {
    (void)d; (void)m; (void)exact;
    unsigned long val = 0;
    if (name && name[0] == '#' && name[1] != '\0') {
        const char *p = name + 1;
        while (*p) {
            val <<= 4;
            if (*p >= '0' && *p <= '9') val |= (*p - '0');
            else if (*p >= 'a' && *p <= 'f') val |= (*p - 'a' + 10);
            else if (*p >= 'A' && *p <= 'F') val |= (*p - 'A' + 10);
            p++;
        }
    }
    scr->pixel = val;
    return 1;
}

static inline GC XCreateGC(Display *d, Drawable dr, unsigned long valmask, void *val) {
    (void)valmask; (void)val;
    if (!d || !dr) return (GC)0;
    unsigned int gcid = d->next_rid++;
    unsigned int p[4];
    p[0] = (4 << 16) | 55; /* X_CreateGC: reqType=55, length=4 */
    p[1] = gcid;
    p[2] = (unsigned int)dr;
    p[3] = 0; /* mask = 0 */
    _xwire_write(d->fd, p, sizeof(p));
    return (GC)(unsigned long)gcid;
}

static inline void XFreeGC(Display *d, GC gc) {
    if (!d || !gc) return;
    unsigned int p[2];
    p[0] = (2 << 16) | 60; /* X_FreeGC: reqType=60, length=2 */
    p[1] = (unsigned int)(unsigned long)gc;
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XSetForeground(Display *d, GC gc, unsigned long pixel) {
    if (!d || !gc) return;
    unsigned int p[4];
    p[0] = (4 << 16) | 56; /* X_ChangeGC: reqType=56, length=4 */
    p[1] = (unsigned int)(unsigned long)gc;
    p[2] = 4; /* GCForeground = (1L<<2) */
    p[3] = (unsigned int)pixel;
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XFillRectangle(Display *d, Drawable dr, GC gc, int x, int y, unsigned int w, unsigned int h) {
    if (!d || !gc || !dr) return;
    unsigned int p[5];
    p[0] = (5 << 16) | 70; /* X_PolyFillRectangle: reqType=70, length=5 */
    p[1] = (unsigned int)dr;
    p[2] = (unsigned int)(unsigned long)gc;
    p[3] = ((int)y << 16) | ((int)x & 0xFFFF);
    p[4] = ((unsigned int)h << 16) | (w & 0xFFFF);
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XDrawString(Display *d, Drawable dr, GC gc, int x, int y, const char *s, int len) {
    if (!d || !dr || !gc || !s || len <= 0 || len > 200) return;
    unsigned int p[64];
    int total_bytes = 16 + 2 + len;
    int pad = (4 - (total_bytes & 3)) & 3;
    total_bytes += pad;
    int words = total_bytes >> 2;

    p[0] = (words << 16) | 74; /* X_PolyText8: reqType=74 */
    p[1] = (unsigned int)dr;
    p[2] = (unsigned int)(unsigned long)gc;
    p[3] = ((int)y << 16) | ((int)x & 0xFFFF);

    unsigned char *bp = (unsigned char *)&p[4];
    bp[0] = (unsigned char)len;
    bp[1] = 0; /* delta */
    for (int i = 0; i < len; i++) bp[2 + i] = s[i];
    for (int i = 0; i < pad; i++) bp[2 + len + i] = 0;

    _xwire_write(d->fd, p, words * 4);
}

static inline Pixmap XCreatePixmap(Display *d, Drawable dr, unsigned int w, unsigned int h, unsigned int depth) {
    if (!d || !dr) return 0;
    unsigned int pid = d->next_rid++;
    unsigned int p[4];
    p[0] = (4 << 16) | (depth << 8) | 53; /* X_CreatePixmap: reqType=53, length=4 */
    p[1] = pid;
    p[2] = (unsigned int)dr;
    p[3] = ((unsigned int)h << 16) | (w & 0xFFFF);
    _xwire_write(d->fd, p, sizeof(p));
    return (Pixmap)pid;
}

static inline void XFreePixmap(Display *d, Pixmap p) {
    if (!d || !p) return;
    unsigned int req[2];
    req[0] = (2 << 16) | 54; /* X_FreePixmap: reqType=54, length=2 */
    req[1] = (unsigned int)p;
    _xwire_write(d->fd, req, sizeof(req));
}

static inline void XDestroyWindow(Display *d, Window w) {
    if (!d || !w) return;
    unsigned int p[2];
    p[0] = (2 << 16) | 4; /* X_DestroyWindow: reqType=4, length=2 */
    p[1] = (unsigned int)w;
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XCopyArea(Display *d, Drawable src, Drawable dst, GC gc, int sx, int sy, unsigned int w, unsigned int h, int dx, int dy) {
    if (!d || !src || !dst || !gc) return;
    unsigned int p[7];
    p[0] = (7 << 16) | 62; /* X_CopyArea: reqType=62, length=7 */
    p[1] = (unsigned int)src;
    p[2] = (unsigned int)dst;
    p[3] = (unsigned int)(unsigned long)gc;
    p[4] = ((int)sy << 16) | ((int)sx & 0xFFFF);
    p[5] = ((int)dy << 16) | ((int)dx & 0xFFFF);
    p[6] = ((unsigned int)h << 16) | (w & 0xFFFF);
    _xwire_write(d->fd, p, sizeof(p));
}

static inline int XGetWindowAttributes(Display *d, Window w, XWindowAttributes *attr) {
    (void)d; (void)w;
    if (!attr) return 0;
    attr->x = 0; attr->y = 0;
    attr->width = 640; attr->height = 480;
    attr->border_width = 1;
    attr->override_redirect = 0;
    attr->map_state = IsViewable;
    return 1;
}

static inline int XQueryTree(Display *d, Window w, Window *root_return, Window *parent_return, Window **children_return, unsigned int *nchildren_return) {
    (void)d; (void)w;
    if (root_return) *root_return = d ? d->root : 0;
    if (parent_return) *parent_return = d ? d->root : 0;
    if (children_return) *children_return = NULL;
    if (nchildren_return) *nchildren_return = 0;
    return 1;
}

static inline int XGetWindowProperty(Display *d, Window w, Atom prop, long off, long len, int del, Atom req_t, Atom *act_t, int *act_f, unsigned long *nitems, unsigned long *bytes_after, unsigned char **prop_ret) {
    (void)d; (void)w; (void)prop; (void)off; (void)len; (void)del; (void)req_t;
    if (act_t) *act_t = 0;
    if (act_f) *act_f = 0;
    if (nitems) *nitems = 0;
    if (bytes_after) *bytes_after = 0;
    if (prop_ret) *prop_ret = NULL;
    return 1;
}

static inline void XChangeProperty(Display *d, Window w, Atom prop, Atom type, int fmt, int mode, const unsigned char *data, int nelem) {
    if (!d || !w || !prop || !data || nelem <= 0) return;
    int bytes_per_elem = (fmt == 32) ? 4 : ((fmt == 16) ? 2 : 1);
    int data_bytes = nelem * bytes_per_elem;
    int pad = (4 - (data_bytes & 3)) & 3;
    int total_bytes = 24 + data_bytes + pad;
    int words = total_bytes >> 2;

    unsigned int p[128];
    if (words > (int)(sizeof(p) / sizeof(p[0]))) return;

    p[0] = (words << 16) | ((mode & 0xFF) << 8) | 18; /* X_ChangeProperty */
    p[1] = (unsigned int)w;
    p[2] = (unsigned int)prop;
    p[3] = (unsigned int)type;
    p[4] = ((unsigned int)fmt) & 0xFF;
    p[5] = (unsigned int)nelem;

    unsigned char *dp = (unsigned char *)&p[6];
    for (int i = 0; i < data_bytes; i++) dp[i] = data[i];
    for (int i = 0; i < pad; i++) dp[data_bytes + i] = 0;

    _xwire_write(d->fd, p, total_bytes);
}

static inline int XGrabPointer(Display *d, Window grab_w, int owner, unsigned int mask, int sync_p, int sync_k, Window confine, unsigned long cursor, Time time) {
    (void)owner; (void)sync_p; (void)sync_k; (void)confine; (void)cursor; (void)time;
    if (!d) return 1;
    unsigned int p[6];
    p[0] = (6 << 16) | 26; /* X_GrabPointer: reqType=26, length=6 */
    p[1] = (unsigned int)grab_w;
    p[2] = (1 << 24) | (1 << 16) | (mask & 0xFFFF);
    p[3] = (unsigned int)confine;
    p[4] = (unsigned int)cursor;
    p[5] = 0;
    _xwire_write(d->fd, p, sizeof(p));
    return 0; /* GrabSuccess */
}

static inline void XUngrabPointer(Display *d, Time time) {
    if (!d) return;
    unsigned int p[2] = { (2 << 16) | 27, (unsigned int)time }; /* X_UngrabPointer */
    _xwire_write(d->fd, p, sizeof(p));
}

static inline int XQueryPointer(Display *d, Window w, Window *r, Window *c, int *rx, int *ry, int *wx, int *wy, unsigned int *mask) {
    (void)d; (void)w;
    if (r) *r = d ? d->root : 0;
    if (c) *c = 0;
    if (rx) *rx = 0;
    if (ry) *ry = 0;
    if (wx) *wx = 0;
    if (wy) *wy = 0;
    if (mask) *mask = 0;
    return 1;
}

static inline void XSendEvent(Display *d, Window w, int prop, long mask, XEvent *ev) {
    if (!d || !ev) return;
    unsigned int p[11];
    p[0] = (11 << 16) | ((prop & 0xFF) << 8) | 25; /* X_SendEvent: reqType=25, length=11 */
    p[1] = (unsigned int)w;
    p[2] = (unsigned int)mask;
    if (ev->type == ClientMessage) {
        unsigned char *ep = (unsigned char *)&p[3];
        for (int i = 0; i < 32; i++) ep[i] = 0;
        ep[0] = 33; /* ClientMessage */
        ep[1] = (unsigned char)ev->xclient.format;
        *(unsigned int *)&ep[4] = (unsigned int)ev->xclient.window;
        *(unsigned int *)&ep[8] = (unsigned int)ev->xclient.message_type;
        for (int i = 0; i < 5; i++)
            *(unsigned int *)&ep[12 + i * 4] = (unsigned int)ev->xclient.data.l[i];
    } else {
        unsigned char *ep = (unsigned char *)&p[3];
        const unsigned char *src = (const unsigned char *)ev;
        for (int i = 0; i < 32; i++) ep[i] = src[i];
    }
    _xwire_write(d->fd, p, sizeof(p));
}

static inline XModifierKeymap *XGetModifierMapping(Display *d) {
    (void)d;
    static KeyCode dummy[8] = { 0, 0, 0, 0, 77 /* Mod2 = NumLock */, 0, 0, 0 };
    static XModifierKeymap m = { 1, dummy };
    return &m;
}
static inline void XFreeModifiermap(XModifierKeymap *m) { (void)m; }

static inline KeyCode XKeysymToKeycode(Display *d, KeySym ks) {
    (void)d;
    if (ks >= XK_1 && ks <= XK_9) return (KeyCode)(10 + (ks - XK_1));
    switch (ks) {
    case XK_Num_Lock: return 77;
    case XK_Return:   return 36;
    case XK_space:    return 65;
    case XK_q:        return 24;
    case XK_w:        return 25;
    case XK_d:        return 40;
    case XK_c:        return 54;
    case XK_h:        return 43;
    case XK_j:        return 44;
    case XK_k:        return 45;
    case XK_l:        return 46;
    case XK_r:        return 27;
    case XK_b:        return 56;
    case XK_v:        return 55;
    case XK_a:        return 38;
    case XK_s:        return 39;
    case XK_f:        return 41;
    default:          return 0;
    }
}

static inline KeySym XkbKeycodeToKeysym(Display *d, KeyCode kc, unsigned int group, unsigned int level) {
    (void)d; (void)group; (void)level;
    if (kc >= 10 && kc <= 18) return XK_1 + (kc - 10);
    if (kc == 36) return XK_Return;
    if (kc == 65) return XK_space;
    if (kc == 24) return XK_q;
    if (kc == 25) return XK_w;
    if (kc == 40) return XK_d;
    if (kc == 54) return XK_c;
    if (kc == 43) return XK_h;
    if (kc == 44) return XK_j;
    if (kc == 45) return XK_k;
    if (kc == 46) return XK_l;
    if (kc == 27) return XK_r;
    if (kc == 56) return XK_b;
    if (kc == 55) return XK_v;
    if (kc == 38) return XK_a;
    if (kc == 39) return XK_s;
    if (kc == 41) return XK_f;
    return 0;
}

/* ========================================================================= */
/* Core Window Manager Implementation                                       */
/* ========================================================================= */

#define LENGTH(X)           (sizeof(X) / sizeof(X)[0])
#define MAX(A,B)            ((A) > (B) ? (A) : (B))
#define MIN(A,B)            ((A) < (B) ? (A) : (B))
#define CLEANMASK(mask)     (mask & ~(numlockmask|LockMask))
#define SELCLIENT(v)        if (!selmon || !selmon->ws->focus || !(v = selmon->ws->focus->u.client)) return

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

enum {
    FLAG_FLOATING   = 1 << 0,
    FLAG_FULLSCREEN = 1 << 1,
    FLAG_DIALOG     = 1 << 2,
};

struct Client {
    Window win;
    unsigned short x, y, w, h;
    unsigned short tile_x, tile_y, tile_w, tile_h;
    short workspace;
    unsigned short flags;
    Client *next;
    Client *prev;
};

struct Node {
    union {
        Client *client;
        struct children { Node *first, *second; } ch;
    } u;
    Node *parent;
    unsigned is_leaf : 1;
    unsigned split   : 1;
};

struct Workspace {
    Node *root;
    Node *focus;
    Client *fs_client;
};

struct Monitor {
    Workspace *ws;
    Window barwin;
    Pixmap barpm;
    Monitor *next;
    int num, x, y, w, h;
    int bar_width;
    Client *fs_client;
    short curtag;
    short next_split;
    unsigned dirty : 1;
};


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

static const int gappx           = 0; //10
static const int borderpx        = 1;
static const char *termcmd[]     = { "st", NULL };
static const char *menucmd[]     = {"/bin/sh", "-c", "/home/$USER/dotfile/dmenuscript", NULL};
/* static const float default_ratio = 0.5f; */
static const char *colors[]      = {
    "#FFFFFF",  /* COL_BORDER_ACTIVE   — borda da janela focada (ouro miasma) */
    "#2d2d2d",  /* COL_BORDER_INACTIVE — borda das janelas não focadas */
    "#000000",  /* COL_BAR_BG          — fundo da barra */
    "#c2c2b0",  /* COL_BAR_FG          — texto do workspace ativo na barra (bege miasma) */
    "#685742",  /* COL_BAR_HL          — texto dos workspaces inativos na barra (marrom miasma) */
};
/*
static const int tagmap[WORKSPACE_COUNT] = {
    1, 1, 1, 1, 1, 1, 1, 1, 0
};
*/
static int tagmap[WORKSPACE_COUNT];
static const unsigned int MODKEY = Mod1Mask;

/* Autostart commands (run once at startup, before the event loop) */
static const char *autostart[][4] = {
    /* { "/bin/sh", "-c", "picom", NULL }, */
   // { "/bin/sh", "-c", "nitrogen --restore", NULL },
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

enum { NODEPOOL = 256 };
static Node  nodepool[NODEPOOL];
static int   nodeidx;
static Node *nodefreelist;
static Client *client_freelist;

char **envp_global;
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
    unsigned long val = (unsigned char)c;
    if (n >= 8) {
        val |= val << 8;
        val |= val << 16;
        val |= val << 32;
        while (n >= 8) {
            *(unsigned long *)p = val;
            p += 8;
            n -= 8;
        }
    }
    while (n--) *p++ = (unsigned char)c;
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
#define HEAP_SIZE (2UL * 1024)          /* 2 KB — bump allocator */
static char heap[HEAP_SIZE];
static unsigned long heap_ptr;

static void *bump_calloc(size_t nmemb, size_t size)
{
    size_t total = (nmemb * size + 7UL) & ~7UL;
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
    n->u.client  = NULL;
    n->parent  = NULL;
    n->u.ch.first   = NULL;
    n->u.ch.second  = NULL;
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

    sibling   = (parent->u.ch.first == n) ? parent->u.ch.second : parent->u.ch.first;
    grandparent = parent->parent;

    sibling->parent = grandparent;
    if (grandparent) {
        if (grandparent->u.ch.first == parent)
            grandparent->u.ch.first = sibling;
        else
            grandparent->u.ch.second = sibling;
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
    newleaf->u.client = c;

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
    newnode->u.ch.first  = focus;
    newnode->u.ch.second = newleaf;
    focus->parent   = newnode;
    newleaf->parent = newnode;

    if (newnode->parent) {
        if (newnode->parent->u.ch.first == focus)
            newnode->parent->u.ch.first = newnode;
        else
            newnode->parent->u.ch.second = newnode;
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
    tmp    = n->u.ch.first;
    n->u.ch.first  = n->u.ch.second;
    n->u.ch.second = tmp;
}

Node *node_in_direction(Node *n, int orient, int dir)
{
    Node *p, *c;
    int want_first = (dir == 0);

    if (!n || !n->is_leaf) return NULL;

    p = n;
    while (p->parent) {
        if (p->parent->split == orient) {
            int on_first = (p->parent->u.ch.first == p);
            if (on_first != want_first) {
                c = want_first ? p->parent->u.ch.first : p->parent->u.ch.second;

                int sub_pos = -1;
                Node *q = n;
                while (q != p->parent) {
                    if (q->parent && q->parent->split != orient)
                        sub_pos = (q->parent->u.ch.first == q) ? 0 : 1;
                    q = q->parent;
                }

                while (!c->is_leaf) {
                    if (c->split != orient && sub_pos >= 0)
                        c = sub_pos ? c->u.ch.second : c->u.ch.first;
                    else
                        c = dir ? c->u.ch.first : c->u.ch.second;
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
    while (!n->is_leaf) n = n->u.ch.first;
    return n;
}

Node *node_find_client(Node *n, Client *c)
{
    Node *f;
    if (!n) return NULL;
    if (n->is_leaf && n->u.client == c)
        return n;
    if (!n->is_leaf) {
        f = node_find_client(n->u.ch.first, c);
        return f ? f : node_find_client(n->u.ch.second, c);
    }
    return NULL;
}

void node_hide_foreach(Node *n)
{
    if (!n) return;
    if (n->is_leaf) {
        if (n->u.client)
            XMoveWindow(dpy, n->u.client->win, -9999, -9999);
    } else {
        node_hide_foreach(n->u.ch.first);
        node_hide_foreach(n->u.ch.second);
    }
}

static void
arrange_floating(Monitor *m)
{
    Client *c;
    Node *n;

    if (!m) return;

    for (c = clients; c; c = c->next) {
        if (!(c->flags & FLAG_FLOATING)) continue;
        n = node_find_client(m->ws->root, c);
        if (n) {
            XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
            if (n != m->ws->focus) {
                XSetWindowBorderWidth(dpy, c->win, borderpx);
                XSetWindowBorder(dpy, c->win, border_inactive);
            }
        } else if (c->workspace == m->curtag) {
            XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
            XSetWindowBorderWidth(dpy, c->win, borderpx);
            XSetWindowBorder(dpy, c->win, border_active);
        } else {
            XMoveWindow(dpy, c->win, -9999, -9999);
        }
    }
}
void arrange_node(Node *n, int x, int y, int w, int h, int gap)
{
    Client *c;
    if (!n) return;

    if (n->is_leaf) {
        c = n->u.client;
        if (!c || (c->flags & (FLAG_FULLSCREEN | FLAG_FLOATING)))
            return;
        XMoveResizeWindow(dpy, c->win, x + borderpx, y + borderpx,
                          MAX(1, w - (borderpx << 1)), MAX(1, h - (borderpx << 1)));
        return;
    }

    int half_gap = gap > 0 ? (gap >> 1) : 0;
    if (n->split == SPLIT_VERTICAL) {
        int fw = MAX(1, (w >> 1) - half_gap);
        arrange_node(n->u.ch.first,  x, y, fw, h, gap);
        arrange_node(n->u.ch.second, x + fw + gap, y, MAX(1, w - fw - gap), h, gap);
    } else {
        int fh = MAX(1, (h >> 1) - half_gap);
        arrange_node(n->u.ch.first,  x, y, w, fh, gap);
        arrange_node(n->u.ch.second, x, y + fh + gap, w, MAX(1, h - fh - gap), gap);
    }
}

void arrange(Monitor *m)
{
    int wx, wy, ww, wh;

    if (!m) return;

    if (m->barwin && !(m->ws->focus && m->ws->focus->u.client &&
        (m->ws->focus->u.client->flags & FLAG_FULLSCREEN))) {
        m->bar_width = m->w - 2 * gappx;
        XMoveResizeWindow(dpy, m->barwin,
                          m->x + gappx, m->y + gappx,
                          m->bar_width, BAR_HEIGHT);
    }

    wx = m->x + gappx;
    wy = m->y + gappx + BAR_HEIGHT + gappx;
    ww = m->w - gappx - borderpx;
    wh = m->h - BAR_HEIGHT - 2 * gappx - borderpx;

    if (m->ws->root)
        arrange_node(m->ws->root, wx, wy, ww, wh, gappx);

    arrange_floating(m);

    Client *fs = m->ws->fs_client;
    if (fs) {
        XMoveResizeWindow(dpy, fs->win, m->x, m->y, m->w, m->h);
        XRaiseWindow(dpy, fs->win);
    }

    if (m->ws->focus && m->ws->focus->u.client && m->ws->focus->u.client != fs) {
        XSetWindowBorder(dpy, m->ws->focus->u.client->win, border_active);
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
    c->workspace = selmon ? selmon->curtag : 0;

    if (XGetWindowProperty(dpy, w, netatom[NET_WM_WINDOW_TYPE], 0L, 2L,
                           False, XA_ATOM, &actual, &format,
                           &n, &left, &data) == Success && data) {
        atoms = (Atom *)data;
        for (unsigned long i = 0; i < n; i++)
            if (atoms[i] == netatom[NET_WM_WINDOW_TYPE_DIALOG])
                c->flags |= FLAG_DIALOG | FLAG_FLOATING;
        XFree(data);
    }

    if (clients) clients->prev = c;
    c->next = clients;
    c->prev = NULL;
    clients = c;

    swa.event_mask = EnterWindowMask | FocusChangeMask | PropertyChangeMask |
                     ButtonPressMask | SubstructureNotifyMask;
    swa.background_pixel = bar_bg;
    XSetWindowBorderWidth(dpy, w, borderpx);
    XSetWindowBorder(dpy, w, border_inactive);
    XChangeWindowAttributes(dpy, w, CWEventMask | CWBackPixel, &swa);
    mask_children(w);

    XGrabButton(dpy, Button1, MODKEY, w, False, ButtonPressMask,
                GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(dpy, Button3, MODKEY, w, False, ButtonPressMask,
                GrabModeAsync, GrabModeAsync, None, None);

    if (!(c->flags & FLAG_FLOATING)) {
        Node *nl = node_insert(selmon, c);
        selmon->dirty = 1;
        arrange(selmon);
        XMapWindow(dpy, w);
        if (nl) focus(selmon, nl);
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
        if (m->ws->fs_client == c)
            m->ws->fs_client = NULL;
        newfocus = node_detach(n, m->ws);
        if (newfocus)
            m->ws->focus = newfocus;
        else
            m->ws->focus = NULL;
        node_free(n);
        m->dirty = 1;
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
    if (!m || !n || !n->u.client) return;
    if (m->ws->focus && m->ws->focus->u.client && m->ws->focus != n)
        XSetWindowBorder(dpy, m->ws->focus->u.client->win, border_inactive);
    m->ws->focus = n;
    XSetWindowBorder(dpy, n->u.client->win, border_active);
    XSetInputFocus(dpy, n->u.client->win, RevertToPointerRoot, CurrentTime);
    XRaiseWindow(dpy, n->u.client->win);
    XChangeProperty(dpy, root, netatom[NET_ACTIVE_WINDOW],
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)&n->u.client->win, 1);
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
    if (!(c->flags & FLAG_FLOATING)) return;

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
            if (selmon && selmon->barwin)
                XRaiseWindow(dpy, selmon->barwin);
            break;
        case ButtonRelease:
            c->x = ev.xbutton.x_root - ox;
            c->y = ev.xbutton.y_root - oy;
            if (c->x + c->w + (borderpx << 1) > selmon->x + selmon->w)
                c->x = selmon->x + selmon->w - c->w - (borderpx << 1);
            if (c->y + c->h + (borderpx << 1) > selmon->y + selmon->h)
                c->y = selmon->y + selmon->h - c->h - (borderpx << 1);
            if (c->x < selmon->x) c->x = selmon->x;
            if (c->y < selmon->y) c->y = selmon->y;
            XMoveWindow(dpy, c->win, c->x, c->y);
            XUngrabPointer(dpy, CurrentTime);
            focus(selmon, selmon->ws->focus);
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
    if (!(c->flags & FLAG_FLOATING)) return;

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
            if (selmon && selmon->barwin)
                XRaiseWindow(dpy, selmon->barwin);
            break;
        case ButtonRelease:
            c->w = MAX(1, ev.xbutton.x_root - c->x);
            c->h = MAX(1, ev.xbutton.y_root - c->y);
            if (c->x + c->w + (borderpx << 1) > selmon->x + selmon->w)
                c->w = selmon->x + selmon->w - c->x - (borderpx << 1);
            if (c->y + c->h + (borderpx << 1) > selmon->y + selmon->h)
                c->h = selmon->y + selmon->h - c->y - (borderpx << 1);
            c->w = MAX(1, c->w);
            c->h = MAX(1, c->h);
            XResizeWindow(dpy, c->win, c->w, c->h);
            XUngrabPointer(dpy, CurrentTime);
            focus(selmon, selmon->ws->focus);
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
    m->bar_width = w;
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

    int nmon = 0;
    for (Monitor *tm = mons; tm; tm = tm->next) nmon++;
    for (int i = 0; i < WORKSPACE_COUNT; i++)
        tagmap[i] = (nmon > 1 && i != WORKSPACE_COUNT - 1) ? 1 : 0;
}

static void
update_bar_visibility(Monitor *m)
{
    if (!m || !m->barwin) return;
    if (m->ws->focus && m->ws->focus->u.client &&
        (m->ws->focus->u.client->flags & FLAG_FULLSCREEN))
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
        if (m->ws->focus && m->ws->focus->u.client)
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
        arrange(om);
    }
    m->dirty = 1;

    if (m->ws->focus && m->ws->focus->u.client)
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

    sw = m->bar_width / ntags;

    if (!bar_gc)
        bar_gc = XCreateGC(dpy, m->barwin, 0, NULL);

    d = m->barpm ? m->barpm : m->barwin;

    XSetForeground(dpy, bar_gc, bar_bg);
    XFillRectangle(dpy, d, bar_gc, 0, 0, m->bar_width, BAR_HEIGHT);

    for (idx = 0; idx < ntags; idx++) {
        i = tags[idx];
        x = idx * sw;
        itoa(i + 1, buf, sizeof(buf));
        XSetForeground(dpy, bar_gc, (m == selmon && m->curtag == i) ? bar_fg : bar_hl);
        XDrawString(dpy, d, bar_gc, x + (sw >> 1) - 4,
                    (BAR_HEIGHT >> 1) + 5, buf, strlen(buf));
    }

    if (m->barpm)
        XCopyArea(dpy, m->barpm, m->barwin, bar_gc, 0, 0,
                  m->bar_width, BAR_HEIGHT, 0, 0);
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
    Window par, *kids, w;
    unsigned int nk;

    if (!c) {
        w = e->xbutton.window;
        while (!c && w != root && w != None) {
            if (!XQueryTree(dpy, w, &(Window){0}, &par, &kids, &nk)) break;
            if (kids) XFree(kids);
            if (par == root || par == None) break;
            c = find_client(par);
            w = par;
        }
    }

    for (m = mons; m; m = m->next) {
        if (e->xbutton.window == m->barwin || e->xbutton.subwindow == m->barwin) {
            int i, ntags = 0, tags[WORKSPACE_COUNT];
            for (i = 0; i < WORKSPACE_COUNT; i++)
                if (tagmap[i] == m->num)
                    tags[ntags++] = i;
            if (ntags > 0) {
                int sw = m->w / ntags;
                int clicked = e->xbutton.x_root / sw;
                if (clicked >= 0 && clicked < ntags) {
                    Arg a = {.i = tags[clicked]};
                    focusworkspace(&a);
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
    Monitor *m;
    Client *c;
    Node *n;
    Window par, *kids, w;
    unsigned int nk;

    c = find_client(ev->window);
    w = ev->window;
    while (!c && w != root && w != None) {
        if (!XQueryTree(dpy, w, &(Window){0}, &par, &kids, &nk)) break;
        if (kids) XFree(kids);
        if (par == root || par == None) break;
        c = find_client(par);
        w = par;
    }
    if (!c) return;

    n = NULL;
    for (m = mons; m; m = m->next) {
        n = node_find_client(m->ws->root, c);
        if (n) break;
    }
    if (!m || !n) return;

    if (m != selmon) {
        Monitor *prev = selmon;
        selmon = m;
        if (prev) { prev->dirty = 1; drawbar(prev); }
        m->dirty = 1;
        drawbar(m);
    }
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

    if (c && (c->flags & FLAG_FLOATING)) {
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
    (void)ee;
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

    c->flags ^= FLAG_FULLSCREEN;
    if (c->flags & FLAG_FULLSCREEN) {
        selmon->ws->fs_client = c;
        c->tile_x = c->x; c->tile_y = c->y;
        c->tile_w = c->w; c->tile_h = c->h;
        XSetWindowBorderWidth(dpy, c->win, 0);
        XMoveResizeWindow(dpy, c->win, selmon->x, selmon->y,
                          selmon->w, selmon->h);
        XUnmapWindow(dpy, selmon->barwin);
        XRaiseWindow(dpy, c->win);
    } else {
        selmon->ws->fs_client = NULL;
        c->x = c->tile_x; c->y = c->tile_y;
        c->w = c->tile_w; c->h = c->tile_h;
        XMapWindow(dpy, selmon->barwin);
        XSetWindowBorderWidth(dpy, c->win, borderpx);
        XSetWindowBorder(dpy, c->win,
                         (c == selmon->ws->focus->u.client) ? border_active : border_inactive);
        selmon->dirty = 1;
        arrange(selmon);
    }
}

void togglefloating(const Arg *arg)
{
    Client *c;
    int fw, fh;
    SELCLIENT(c);

    c->flags ^= FLAG_FLOATING;
    if (c->flags & FLAG_FLOATING) {
        c->tile_x = c->x; c->tile_y = c->y;
        c->tile_w = c->w; c->tile_h = c->h;
        fw = (selmon->w * 3) / 5;
        fh = (selmon->h * 3) / 5;
        c->x = selmon->x + ((selmon->w - fw) >> 1);
        c->y = selmon->y + ((selmon->h - fh) >> 1);
        if (c->x + fw + (borderpx << 1) > selmon->x + selmon->w)
            c->x = selmon->x + selmon->w - fw - (borderpx << 1);
        if (c->y + fh + (borderpx << 1) > selmon->y + selmon->h)
            c->y = selmon->y + selmon->h - fh - (borderpx << 1);
        if (c->x < selmon->x) c->x = selmon->x;
        if (c->y < selmon->y) c->y = selmon->y;
        c->w = fw; c->h = fh;
        XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
        XSetWindowBorderWidth(dpy, c->win, borderpx);
        XSetWindowBorder(dpy, c->win, border_active);
        XRaiseWindow(dpy, c->win);
    } else {
        c->x = c->tile_x; c->y = c->tile_y;
        c->w = c->tile_w; c->h = c->tile_h;
        if (!node_find_client(selmon->ws->root, c))
            node_insert(selmon, c);
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
            if ((!n || !n->u.client) && om->ws->root)
                n = node_first_leaf(om->ws->root);
            if (n && n->u.client) {
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

static Node *workspace_insert(Workspace *ws, Client *c, int split)
{
    Node *nl = node_new();
    nl->u.client = c;

    if (!ws->root) {
        ws->root = nl;
        ws->focus = nl;
    } else {
        Node *ff = ws->focus ? ws->focus : node_first_leaf(ws->root);
        Node *nn = node_new();
        nn->is_leaf = 0;
        nn->split = split;
        nn->u.ch.first  = ff;
        nn->u.ch.second = nl;
        nn->parent = ff->parent;
        if (ff->parent) {
            if (ff->parent->u.ch.first == ff)
                ff->parent->u.ch.first = nn;
            else
                ff->parent->u.ch.second = nn;
        } else {
            ws->root = nn;
        }
        ff->parent = nn;
        nl->parent = nn;
        ws->focus = nl;
    }
    return nl;
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
        Client *c = n->u.client;
        int tag = raw;
        if (!c) return;

        Node *newfocus = node_detach(n, ws);
        ws->focus = newfocus;
        node_free(n);

        c->workspace = tag;
        workspace_insert(&workspaces[tag], c, SPLIT_VERTICAL);
        if (tag != selmon->curtag) {
            XMoveWindow(dpy, c->win, -9999, -9999);
            if (!newfocus) {
                XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
                XChangeProperty(dpy, root, netatom[NET_ACTIVE_WINDOW],
                                XA_WINDOW, 32, PropModeReplace,
                                (unsigned char *)&(Window){0}, 1);
            }
        }
        arrange(selmon);
        return;
    }

    raw -= 100;
    orient = raw >> 1;
    dir    = raw & 1;

    target = node_in_direction(n, orient, dir);
    if (target && target->is_leaf && target->u.client && n->u.client) {
        tmpc = n->u.client;
        n->u.client = target->u.client;
        target->u.client = tmpc;
        focus(selmon, target);
        arrange(selmon);
        return;
    }

    if (!n->u.client) return;
    {
        Monitor *om = monitor_in_direction(selmon, orient, dir);
        if (om) {
            Client *c = n->u.client;

            Node *newfocus = node_detach(n, ws);
            ws->focus = newfocus;
            node_free(n);

            Node *nl = workspace_insert(om->ws, c, orient);

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
