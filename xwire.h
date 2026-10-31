#ifndef XWIRE_H
#define XWIRE_H

#include <sys/socket.h>
#include <sys/un.h>

#ifndef NULL
#define NULL ((void *)0)
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

#endif /* XWIRE_H */
