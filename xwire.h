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
    unsigned char body[2048];
    unsigned int to_read = (add_len * 4) > sizeof(body) ? sizeof(body) : (add_len * 4);
    _xwire_read(fd, body, to_read);

    _global_display.fd = fd;
    _global_display.rid_base = *(unsigned int *)&body[4];
    _global_display.rid_mask = *(unsigned int *)&body[8];
    _global_display.next_rid = _global_display.rid_base;

    unsigned short vendor_len = *(unsigned short *)&body[16];
    unsigned char num_formats = body[21];
    int screen_offset = 32 + ((vendor_len + 3) & ~3) + (num_formats * 8);

    if (screen_offset + 40 <= (int)to_read) {
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
    p[5] = (border_width << 16) | 1; /* class = InputOutput */
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
    p[0] = (4 << 16) | (1 << 8) | 33;
    p[1] = (unsigned int)w;
    p[2] = (mod << 16) | (keycode & 0xFF);
    p[3] = 0;
    _xwire_write(d->fd, p, sizeof(p));
}

static inline void XUngrabKey(Display *d, int keycode, unsigned int mod, Window w) {
    (void)d; (void)keycode; (void)mod; (void)w;
}

static inline void XGrabButton(Display *d, unsigned int btn, unsigned int mod, Window w, int owner, unsigned int mask, int sync_p, int sync_k, Window confine, unsigned long cursor) {
    (void)owner; (void)sync_p; (void)sync_k; (void)confine; (void)cursor;
    unsigned int p[6];
    p[0] = (6 << 16) | (owner << 8) | 28;
    p[1] = (unsigned int)w;
    p[2] = (mod << 16) | (mask & 0xFFFF);
    p[3] = (btn & 0xFF);
    p[4] = 0;
    p[5] = 0;
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

        int n = _xwire_read(d->fd, buf, 32);
        if (n <= 0) { e->type = 0; return 1; }

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
        } else if (code == 17) {
            e->type = DestroyNotify;
            e->xdestroywindow.window = *(unsigned int *)&buf[8];
            return 0;
        } else if (code == 12) {
            e->type = Expose;
            e->xexpose.window = *(unsigned int *)&buf[4];
            return 0;
        } else if (code == 4) {
            e->type = ButtonPress;
            e->xbutton.window = *(unsigned int *)&buf[12];
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
    (void)d; (void)only_if_exists;
    unsigned long h = 5381;
    while (*name) h = ((h << 5) + h) + (unsigned char)*name++;
    return (Atom)h;
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

static inline GC XCreateGC(Display *d, Drawable dr, unsigned long valmask, void *val) { (void)d; (void)dr; (void)valmask; (void)val; return (GC)1; }
static inline void XFreeGC(Display *d, GC gc) { (void)d; (void)gc; }
static inline void XSetForeground(Display *d, GC gc, unsigned long pixel) { (void)d; (void)gc; (void)pixel; }
static inline void XFillRectangle(Display *d, Drawable dr, GC gc, int x, int y, unsigned int w, unsigned int h) { (void)d; (void)dr; (void)gc; (void)x; (void)y; (void)w; (void)h; }
static inline void XDrawString(Display *d, Drawable dr, GC gc, int x, int y, const char *s, int len) { (void)d; (void)dr; (void)gc; (void)x; (void)y; (void)s; (void)len; }
static inline Pixmap XCreatePixmap(Display *d, Drawable dr, unsigned int w, unsigned int h, unsigned int depth) { (void)d; (void)dr; (void)w; (void)h; (void)depth; return 0; }
static inline void XFreePixmap(Display *d, Pixmap p) { (void)d; (void)p; }
static inline void XDestroyWindow(Display *d, Window w) { (void)d; (void)w; }
static inline void XCopyArea(Display *d, Drawable src, Drawable dst, GC gc, int sx, int sy, unsigned int w, unsigned int h, int dx, int dy) { (void)d; (void)src; (void)dst; (void)gc; (void)sx; (void)sy; (void)w; (void)h; (void)dx; (void)dy; }

static inline int XGetWindowAttributes(Display *d, Window w, XWindowAttributes *attr) {
    (void)d; (void)w;
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
    if (children_return) *children_return = (void *)0;
    if (nchildren_return) *nchildren_return = 0;
    return 1;
}

static inline int XGetWindowProperty(Display *d, Window w, Atom prop, long off, long len, int del, Atom req_t, Atom *act_t, int *act_f, unsigned long *nitems, unsigned long *bytes_after, unsigned char **prop_ret) {
    (void)d; (void)w; (void)prop; (void)off; (void)len; (void)del; (void)req_t; (void)act_t; (void)act_f; (void)nitems; (void)bytes_after;
    *prop_ret = (void *)0;
    return 0;
}

static inline void XChangeProperty(Display *d, Window w, Atom prop, Atom type, int fmt, int mode, const unsigned char *data, int nelem) {
    (void)d; (void)w; (void)prop; (void)type; (void)fmt; (void)mode; (void)data; (void)nelem;
}

static inline void XSendEvent(Display *d, Window w, int prop, long mask, XEvent *ev) {
    (void)d; (void)w; (void)prop; (void)mask; (void)ev;
}

static inline int XGrabPointer(Display *d, Window grab_w, int owner, unsigned int mask, int sync_p, int sync_k, Window confine, unsigned long cursor, Time time) {
    (void)d; (void)grab_w; (void)owner; (void)mask; (void)sync_p; (void)sync_k; (void)confine; (void)cursor; (void)time;
    return 0;
}
static inline void XUngrabPointer(Display *d, Time time) { (void)d; (void)time; }
static inline int XQueryPointer(Display *d, Window w, Window *r, Window *c, int *rx, int *ry, int *wx, int *wy, unsigned int *mask) {
    (void)d; (void)w; (void)r; (void)c; (void)rx; (void)ry; (void)wx; (void)wy; (void)mask;
    return 1;
}

static inline XModifierKeymap *XGetModifierMapping(Display *d) {
    (void)d;
    static KeyCode dummy[8] = { 0 };
    static XModifierKeymap m = { 1, dummy };
    return &m;
}
static inline void XFreeModifiermap(XModifierKeymap *m) { (void)m; }
static inline KeyCode XKeysymToKeycode(Display *d, KeySym ks) { (void)d; (void)ks; return 0; }

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
