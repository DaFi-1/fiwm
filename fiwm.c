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
#define SYS_RT_SIGACTION 13
#define SYS_SOCKET   41
#define SYS_CONNECT  42
#define SYS_FORK     57
#define SYS_EXECVE   59
#define SYS_EXIT     60
#define SYS_WAIT4    61
#define SYS_KILL     62
#define SYS_READLINK 89
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

static inline long syscall4(long nr, long a1, long a2, long a3, long a4)
{
    long ret;
    register long r10 __asm__("r10") = a4;
    __asm__ __volatile__("syscall"
                 : "=a"(ret)
                 : "a"(nr), "D"(a1), "S"(a2), "d"(a3), "r"(r10)
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

#define _xwire_sys3(n,a,b,c) syscall3((n),(a),(b),(c))
#define sys_write(f,b,c)   syscall3(SYS_WRITE, (long)(f), (long)(b), (long)(c))
#define sys_fork()         syscall3(SYS_FORK, 0, 0, 0)
#define sys_execve(p,a,e)  syscall3(SYS_EXECVE, (long)(p), (long)(a), (long)(e))
#define sys_close(f)       syscall3(SYS_CLOSE, (long)(f), 0, 0)
#define sys_setsid()       syscall3(SYS_SETSID, 0, 0, 0)
#define sys_kill(p,s)      syscall3(SYS_KILL, (long)(p), (long)(s), 0)

#define sys_wait4(p,w,o,r) syscall4(SYS_WAIT4, (long)(p), (long)(w), (long)(o), (long)(r))

/* Types matching X11 API */
typedef unsigned long  Window;
typedef unsigned long  Drawable;
typedef unsigned long  Pixmap;
typedef unsigned long  Colormap;
typedef unsigned long  Atom;
typedef unsigned long  VisualID;
typedef unsigned long  Time;

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
#define KeyPress                 2
#define ButtonPress              4
#define ButtonRelease            5
#define MotionNotify             6
#define EnterNotify              7
#define Expose                   12
#define CreateNotify             16
#define DestroyNotify            17
#define UnmapNotify              18
#define MapRequest               20
#define ConfigureRequest         23
#define ClientMessage            33

#define ButtonPressMask          (1L<<2)
#define ButtonReleaseMask        (1L<<3)
#define EnterWindowMask          (1L<<4)
#define PointerMotionMask        (1L<<6)
#define ExposureMask             (1L<<15)
#define StructureNotifyMask      (1L<<17)
#define SubstructureNotifyMask   (1L<<19)
#define SubstructureRedirectMask (1L<<20)
#define FocusChangeMask          (1L<<21)
#define PropertyChangeMask       (1L<<22)
#define NoEventMask              0L

#define NotifyNormal             0
#define NotifyInferior           2

#define CurrentTime              0L
#define AnyButton                0
#define AnyKey                   0
#define AnyModifier              (1<<15)
#define GrabModeAsync            1
#define None                     0L
#define True                     1
#define False                    0
#define Success                  0
#define GrabSuccess              0
#define PropModeReplace          0
#define RevertToPointerRoot      1
#define IsViewable               2

#define CopyFromParent           0L
#define InputOutput              1
#define CWOverrideRedirect       (1L<<9)

#define ShiftMask                (1<<0)
#define LockMask                 (1<<1)
#define Mod1Mask                 (1<<3)

#define Button1                  1
#define Button3                  3

#define CWBackPixel              (1L<<1)
#define CWEventMask              (1L<<11)
#define CWCursor                 (1L<<14)

#define XA_ATOM                  4UL
#define XA_CARDINAL              6UL
#define XA_WINDOW                33UL

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
    Window parent;
    Window window;
    int vals[5];
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
    XConfigureRequestEvent xconfigurerequest;
    XMapRequestEvent xmaprequest;
    XDestroyWindowEvent xdestroywindow;
    XExposeEvent xexpose;
    XClientMessageEvent xclient;
} XEvent;


typedef struct {
    int fd;
    Window root_window;
    int width, height;
    unsigned int rid_base;
    unsigned int rid_mask;
    unsigned int next_rid;
} Display;

register Display *dpy __asm__("r12");

/* Syscall wrappers */
static char _xwire_outbuf[1024];
static unsigned long _xwire_outlen = 0;
static unsigned char _xwire_inbuf[256];
static int _xwire_in_len = 0;
static int _xwire_in_pos = 0;

static inline void _xwire_flush(int fd) {
    if (_xwire_outlen > 0) {
        _xwire_sys3(SYS_WRITE, fd, (long)_xwire_outbuf, _xwire_outlen);
        _xwire_outlen = 0;
    }
}

static inline void *_xwire_reserve(int fd, unsigned long n) {
    if (__builtin_expect(_xwire_outlen + n > sizeof(_xwire_outbuf), 0)) {
        _xwire_flush(fd);
    }
    void *p = _xwire_outbuf + _xwire_outlen;
    _xwire_outlen += n;
    return p;
}

static inline int _xwire_read(int fd, void *b, unsigned long n) {
    _xwire_flush(fd);
    unsigned char *dst = (unsigned char *)b;
    unsigned long done = 0;
    unsigned long avail = (unsigned long)(_xwire_in_len - _xwire_in_pos);
    if (avail > 0) {
        unsigned long take = (avail < n) ? avail : n;
        __builtin_memcpy(dst, _xwire_inbuf + _xwire_in_pos, take);
        _xwire_in_pos += (int)take;
        done = take;
    }
    while (done < n) {
        int r = (int)_xwire_sys3(SYS_READ, fd, (long)(dst + done), n - done);
        if (r <= 0) break;
        done += r;
    }
    return (int)done;
}

static Display _global_display;

char **envp_global = 0;
static const char *env_display = ":0";
static const char *env_path = "/usr/local/bin:/usr/bin:/bin";
static long xorg_pid = 0;

static void start_xorg(void)
{
    char vt_arg[16] = {0};
    char tty_path[32];
    long len = syscall3(SYS_READLINK, (long)"/proc/self/fd/0", (long)tty_path, sizeof(tty_path) - 1);
    if (len > 8 && tty_path[0] == '/' && *(int *)&tty_path[5] == 0x30797474 /* "tty0" check prefix */) {
        vt_arg[0] = 'v'; vt_arg[1] = 't';
        int p = 8, q = 2;
        while (tty_path[p] >= '0' && tty_path[p] <= '9' && q < 14)
            vt_arg[q++] = tty_path[p++];
    }

    const char *x_argv[] = { "/usr/bin/X", ":0", "-ac", vt_arg[0] ? vt_arg : "-keeptty", vt_arg[0] ? "-keeptty" : NULL, NULL };

    long pid = sys_fork();
    if (pid == 0) {
        sys_execve(x_argv[0], (char *const *)x_argv, envp_global);
        sys_exit(1);
    }
    if (pid > 0) {
        xorg_pid = pid;
    }
}

static inline Display *XOpenDisplay(void) {
    int fd = (int)_xwire_sys3(SYS_SOCKET, AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return (void *)0;

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    char path[64] = "/tmp/.X11-unix/X0";

    const char *dstr = env_display;

    if (dstr && dstr[0] == ':') {
        int i = 0, j = 16;
        while (dstr[++i] && dstr[i] != '.' && j < 60)
            path[j++] = dstr[i];
        path[j] = '\0';
    }

    __builtin_memcpy(addr.sun_path, path, sizeof(path));

    if (_xwire_sys3(SYS_CONNECT, fd, (long)&addr, sizeof(addr)) < 0) {
        _xwire_sys3(SYS_CLOSE, fd, 0, 0);
        /* If connecting to :0 and no X server is running yet, spawn Xorg directly */
        if ((!dstr || (dstr[0] == ':' && dstr[1] == '0' && (dstr[2] == '\0' || dstr[2] == '.'))) && xorg_pid == 0) {
            start_xorg();
            if (xorg_pid > 0) {
                for (int attempt = 0; attempt < 150; attempt++) {
                    syscall3(SYS_POLL, 0, 0, 20); /* sleep 20ms */
                    fd = (int)syscall3(SYS_SOCKET, AF_UNIX, SOCK_STREAM, 0);
                    if (fd >= 0) {
                        if (syscall3(SYS_CONNECT, fd, (long)&addr, sizeof(addr)) == 0) {
                            goto connected;
                        }
                        syscall3(SYS_CLOSE, fd, 0, 0);
                    }
                }
            }
        }
        return (void *)0;
    }
connected: ;

    unsigned char setup_req[12] = { 'l', 0, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    _xwire_sys3(SYS_WRITE, fd, (long)setup_req, 12);

    unsigned char rep_hdr[8];
    if (_xwire_read(fd, rep_hdr, 8) != 8 || rep_hdr[0] != 1) {
        _xwire_sys3(SYS_CLOSE, fd, 0, 0);
        return (void *)0;
    }

    unsigned short add_len = *(unsigned short *)&rep_hdr[6];
    unsigned int total_body = (unsigned int)add_len << 2;
    unsigned int first_chunk = total_body > sizeof(_xwire_outbuf) ? sizeof(_xwire_outbuf) : total_body;
    unsigned int read_bytes = 0;
    while (read_bytes < first_chunk) {
        int r = _xwire_read(fd, (unsigned char *)_xwire_outbuf + read_bytes, first_chunk - read_bytes);
        if (r <= 0) break;
        read_bytes += (unsigned int)r;
    }

    unsigned char *body = (unsigned char *)_xwire_outbuf;
    _global_display.fd = fd;
    _global_display.rid_base = *(unsigned int *)&body[4];
    _global_display.rid_mask = *(unsigned int *)&body[8];
    _global_display.next_rid = _global_display.rid_base;

    unsigned short vendor_len = *(unsigned short *)&body[16];
    unsigned char num_formats = body[21];
    int screen_offset = 32 + ((vendor_len + 3) & ~3) + (num_formats * 8);

    if (screen_offset + 40 <= (int)first_chunk) {
        _global_display.root_window   = *(unsigned int *)&body[screen_offset];
        _global_display.width  = *(unsigned short *)&body[screen_offset + 20];
        _global_display.height = *(unsigned short *)&body[screen_offset + 22];
    } else {
        _global_display.root_window   = 0x100;
        _global_display.width  = 1024;
        _global_display.height = 768;
    }

    /* Drain any remaining reply data into _xwire_inbuf */
    while (read_bytes < total_body) {
        unsigned int want = (total_body - read_bytes) > sizeof(_xwire_inbuf) ? sizeof(_xwire_inbuf) : (total_body - read_bytes);
        int r = _xwire_read(fd, _xwire_inbuf, want);
        if (r <= 0) break;
        read_bytes += (unsigned int)r;
    }

    return &_global_display;
}

static inline void XCloseDisplay(Display *d) {
    if (d && d->fd >= 0) {
        _xwire_sys3(SYS_CLOSE, d->fd, 0, 0);
        d->fd = -1;
    }
}

#define ConnectionNumber(d) ((d)->fd)
#define XDefaultScreen(d) 0
#define DisplayWidth(d, s) ((d) ? (d)->width : 1024)
#define DisplayHeight(d, s) ((d) ? (d)->height : 768)
#define RootWindow(d, s) ((d) ? (d)->root_window : 0)
#define DefaultColormap(d, s) 0
#define DefaultDepth(d, s) 24

static inline void configure_window_1d(Display *d, Window w, unsigned int mask, unsigned int val) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 16);
    p[0] = (4 << 16) | 12;
    p[1] = (unsigned int)w;
    p[2] = mask;
    p[3] = val;
}
#define XRaiseWindow(d, w) configure_window_1d((d), (w), 64, 0)
#define XLowerWindow(d, w) configure_window_1d((d), (w), 64, 1)
#define XSetWindowBorderWidth(d, w, bw) configure_window_1d((d), (w), 16, (unsigned int)(bw))

static inline void XConfigureWindowFromReq(Display *d, XConfigureRequestEvent *ev) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 40);
    int idx = 3;
    unsigned long valmask = ev->value_mask;
    p[1] = (unsigned int)ev->window;
    p[2] = (unsigned int)valmask;
    for (int i = 0; i < 5; i++)
        if (valmask & (1 << i)) p[idx++] = (unsigned int)ev->vals[i];
    p[0] = (idx << 16) | 12;
    _xwire_outlen -= (unsigned long)((10 - idx) << 2);
}

static inline void change_window_attributes_1d(Display *d, Window w, unsigned int mask, unsigned int val) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 16);
    p[0] = (4 << 16) | 2;
    p[1] = (unsigned int)w;
    p[2] = mask;
    p[3] = val;
}
#define XSelectInput(d, w, mask) change_window_attributes_1d((d), (w), 0x800, (unsigned int)(mask))
#define XSetWindowBorder(d, w, pixel) change_window_attributes_1d((d), (w), 8, (unsigned int)(pixel))

static inline void client_set_attributes(Display *d, Window w, unsigned long bg, unsigned long border) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 24);
    p[0] = (6 << 16) | 2;
    p[1] = (unsigned int)w;
    p[2] = (1 << 1) | (1 << 3) | (1 << 11);
    p[3] = (unsigned int)bg;
    p[4] = (unsigned int)border;
    p[5] = EnterWindowMask | FocusChangeMask | PropertyChangeMask | StructureNotifyMask;
}

static inline void XMoveResizeWindow(Display *d, Window w, int x, int y, unsigned int width, unsigned int height) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 28);
    p[0] = (7 << 16) | 12;
    p[1] = (unsigned int)w;
    p[2] = 15;
    p[3] = (unsigned int)x;
    p[4] = (unsigned int)y;
    p[5] = width;
    p[6] = height;
}

static inline void configure_window_2d(Display *d, Window w, unsigned int mask, int v1, int v2) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 20);
    p[0] = (5 << 16) | 12;
    p[1] = (unsigned int)w;
    p[2] = mask;
    p[3] = (unsigned int)v1;
    p[4] = (unsigned int)v2;
}

#define XMoveWindow(d, w, x, y) configure_window_2d((d), (w), 3, (int)(x), (int)(y))
#define XResizeWindow(d, w, width, height) configure_window_2d((d), (w), 12, (int)(width), (int)(height))
#define hide_window(d, w) configure_window_2d((d), (w), 3, -9999, -9999)

static inline void xwire_op_resource(Display *d, unsigned int opcode, unsigned int id) {
    if (!d) return;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 8);
    p[0] = (2 << 16) | opcode;
    p[1] = id;
}

#define XMapWindow(d, w)      xwire_op_resource((d), 8, (unsigned int)(w))
#define XUnmapWindow(d, w)    xwire_op_resource((d), 10, (unsigned int)(w))
#define XDestroyWindow(d, w)  xwire_op_resource((d), 4, (unsigned int)(w))
#define ungrab_mouse(d)       xwire_op_resource((d), 27, 0)



static inline void XGrabKey(Display *d, int keycode, unsigned int mod) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 16);
    p[0] = (4 << 16) | (1 << 8) | 33; /* X_GrabKey: reqType=33, ownerEvents=1, length=4 */
    p[1] = (unsigned int)d->root_window;
    p[2] = (1 << 24) | (((unsigned int)(keycode & 0xFF)) << 16) | (mod & 0xFFFF);
    p[3] = 1;
}



static inline void XGrabButton(Display *d, unsigned int btn, unsigned int mod, Window w) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 24);
    p[0] = (6 << 16) | 28;
    p[1] = (unsigned int)w;
    p[2] = (1 << 24) | (1 << 16) | (ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
    p[3] = 0;
    p[4] = 0;
    p[5] = ((mod & 0xFFFF) << 16) | (btn & 0xFF);
}

#define XFlush(d) _xwire_flush((d)->fd)
#define XSync(d, discard) ((void)(discard), _xwire_flush((d)->fd))

static inline int XNextEvent(Display *d, XEvent *e) {
    for (;;) {
        if (_xwire_in_pos + 32 > _xwire_in_len) {
            _xwire_flush(d->fd);
            _xwire_in_pos = 0;
            _xwire_in_len = 0;

            int n = (int)_xwire_sys3(SYS_READ, d->fd, (long)_xwire_inbuf, sizeof(_xwire_inbuf));
            if (n <= 0) { e->type = 0; return 1; }
            _xwire_in_len = n;
            if (_xwire_in_len < 32) continue;
        }

        const unsigned char *buf = _xwire_inbuf + _xwire_in_pos;
        _xwire_in_pos += 32;

        unsigned char code = buf[0] & 0x7F;
        e->type = (int)code;
        switch (code) {
        case 2: /* KeyPress */
        case 4: /* ButtonPress */
        case 5: /* ButtonRelease */
        case 6: /* MotionNotify */
            {
                unsigned long wins = *(const unsigned long *)&buf[12];
                e->xkey.window = (unsigned int)wins;
                e->xkey.subwindow = (unsigned int)(wins >> 32);
                unsigned int roots = *(const unsigned int *)&buf[20];
                e->xkey.x_root = (short)roots;
                e->xkey.y_root = (short)(roots >> 16);
                e->xkey.keycode = buf[1];
                e->xkey.state = *(const unsigned short *)&buf[28];
            }
            return 0;
        case 20: /* MapRequest */
            e->xmaprequest.window = *(const unsigned int *)&buf[8];
            return 0;
        case 23: /* ConfigureRequest */
            e->xconfigurerequest.window = *(const unsigned int *)&buf[8];
            e->xconfigurerequest.vals[0] = *(const short *)&buf[16];
            e->xconfigurerequest.vals[1] = *(const short *)&buf[18];
            e->xconfigurerequest.vals[2] = *(const unsigned short *)&buf[20];
            e->xconfigurerequest.vals[3] = *(const unsigned short *)&buf[22];
            e->xconfigurerequest.vals[4] = *(const unsigned short *)&buf[24];
            e->xconfigurerequest.value_mask = *(const unsigned short *)&buf[28];
            return 0;
        case 17: /* DestroyNotify */
        case 18: /* UnmapNotify */
            e->xunmap.window = *(const unsigned int *)&buf[8];
            return 0;
        case 12: /* Expose */
            e->xexpose.window = *(const unsigned int *)&buf[4];
            return 0;
        case 7: /* EnterNotify */
            e->xcrossing.window = *(const unsigned int *)&buf[12];
            e->xcrossing.mode = (int)buf[30];
            e->xcrossing.detail = (int)buf[1];
            return 0;
        default:
            return 0;
        }
    }
}


static GC bar_gc;

static inline GC XCreateGC(Display *d, Drawable dr) {
    unsigned int gcid = d->next_rid++;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 16);
    p[0] = (4 << 16) | 55; /* X_CreateGC: reqType=55, length=4 */
    p[1] = gcid;
    p[2] = (unsigned int)dr;
    p[3] = 0; /* mask = 0 */
    return (GC)(unsigned long)gcid;
}



static inline void XSetForeground(Display *d, unsigned long pixel) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 16);
    p[0] = (4 << 16) | 56; /* X_ChangeGC: reqType=56, length=4 */
    p[1] = (unsigned int)(unsigned long)bar_gc;
    p[2] = 4; /* GCForeground = (1L<<2) */
    p[3] = (unsigned int)pixel;
}

static inline void XFillRectangle(Display *d, Drawable dr, int x, int y, unsigned int w, unsigned int h) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 20);
    p[0] = (5 << 16) | 70; /* X_PolyFillRectangle: reqType=70, length=5 */
    p[1] = (unsigned int)dr;
    p[2] = (unsigned int)(unsigned long)bar_gc;
    p[3] = ((int)y << 16) | ((int)x & 0xFFFF);
    p[4] = ((unsigned int)h << 16) | (w & 0xFFFF);
}

static inline void XDrawChar(Display *d, Drawable dr, int x, int y, char ch) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 24);
    p[0] = (6 << 16) | 74; /* X_PolyText8: reqType=74, length=6 */
    p[1] = (unsigned int)dr;
    p[2] = (unsigned int)(unsigned long)bar_gc;
    p[3] = ((int)y << 16) | ((int)x & 0xFFFF);
    p[4] = (1 << 0) | (((unsigned int)(unsigned char)ch) << 16);
    p[5] = 0;
}

static inline Pixmap XCreatePixmap(Display *d, Drawable dr, unsigned int w, unsigned int h, unsigned int depth) {
    if (!d || !dr) return 0;
    unsigned int pid = d->next_rid++;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 16);
    p[0] = (4 << 16) | (depth << 8) | 53; /* X_CreatePixmap: reqType=53, length=4 */
    p[1] = pid;
    p[2] = (unsigned int)dr;
    p[3] = ((unsigned int)h << 16) | (w & 0xFFFF);
    return (Pixmap)pid;
}

static inline void XCopyArea(Display *d, Drawable src, Drawable dst, unsigned int w, unsigned int h) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 28);
    p[0] = (7 << 16) | 62; /* X_CopyArea: reqType=62, length=7 */
    p[1] = (unsigned int)src;
    p[2] = (unsigned int)dst;
    p[3] = (unsigned int)(unsigned long)bar_gc;
    p[4] = 0;
    p[5] = 0;
    p[6] = ((unsigned int)h << 16) | (w & 0xFFFF);
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

static inline void XChangeProperty32(Display *d, Window w, Atom prop, Atom type, const void *data, int nelem) {
    int words = 6 + nelem;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, words << 2);
    p[0] = (words << 16) | 18; /* X_ChangeProperty: mode=PropModeReplace (0) */
    p[1] = (unsigned int)w;
    p[2] = (unsigned int)prop;
    p[3] = (unsigned int)type;
    p[4] = 32;
    p[5] = (unsigned int)nelem;
    const unsigned int *src = (const unsigned int *)data;
    for (int i = 0; i < nelem; i++) p[6 + i] = src[i];
}

static inline int grab_mouse_motion(Display *d) {
    if (!d) return 1;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 24);
    p[0] = (6 << 16) | 26; /* X_GrabPointer: reqType=26, length=6 */
    p[1] = (unsigned int)d->root_window;
    p[2] = (1 << 24) | (1 << 16) | ((PointerMotionMask | ButtonReleaseMask) & 0xFFFF);
    p[3] = 0;
    p[4] = 0;
    p[5] = 0;
    _xwire_flush(d->fd);
    return 0; /* GrabSuccess == 0 */
}








/* ========================================================================= */
/* Core Window Manager Implementation                                       */
/* ========================================================================= */

#define LENGTH(X)           (sizeof(X) / sizeof(X)[0])
#define MAX(A,B)            ((A) > (B) ? (A) : (B))
#define CLEANMASK(mask)     ((mask) & ~((1 << 4) | LockMask))


enum { SPLIT_VERTICAL, SPLIT_HORIZONTAL };
enum { COL_BORDER_ACTIVE, COL_BORDER_INACTIVE, COL_BAR_BG, COL_BAR_FG, COL_BAR_HL };
enum { WORKSPACE_COUNT = 9 };
enum { BAR_HEIGHT = 18 };
enum {
    WM_PROTOCOLS, WM_DELETE_WINDOW,
    NET_SUPPORTED, NET_NUMBER_OF_DESKTOPS,
    NET_WM_STATE, NET_WM_STATE_FULLSCREEN, NET_ACTIVE_WINDOW,
    ATOM_LAST
};

typedef union {
    int i;
    unsigned int ui;
    const void *v;
} Arg;

typedef struct Client Client;
typedef struct Node Node;
typedef struct Workspace Workspace;
typedef struct Monitor Monitor;


enum {
    FLAG_FLOATING   = 1 << 0,
    FLAG_FULLSCREEN = 1 << 1,
};

typedef struct {
    unsigned short x, y, w, h;
} Rect;

struct Client {
    Window win;
    Node *node;
    Client *next;
    union {
        Rect r;
        unsigned long geom_u64;
    } g;
    signed char workspace;
    unsigned char flags;
};

#define TAG_LEAF  0x01UL
#define TAG_SPLIT 0x02UL
#define TAG_MASK  0x03UL
#define PTR_MASK  (~TAG_MASK)

struct Node {
    union {
        Client *client;
        struct children { Node *first, *second; } ch;
    } u;
    unsigned long tagged_parent;
};

#define NODE_PARENT(n) ((Node *)((n)->tagged_parent & PTR_MASK))
#define NODE_SET_PARENT(n, p) do { (n)->tagged_parent = ((unsigned long)(p) & PTR_MASK) | ((n)->tagged_parent & TAG_MASK); } while(0)
#define NODE_IS_LEAF(n) (((n)->tagged_parent & TAG_LEAF) != 0)
#define NODE_SET_LEAF(n, v) do { if (v) (n)->tagged_parent |= TAG_LEAF; else (n)->tagged_parent &= ~TAG_LEAF; } while(0)
#define NODE_SPLIT(n) (((n)->tagged_parent & TAG_SPLIT) ? 1 : 0)
#define NODE_SET_SPLIT(n, v) do { if (v) (n)->tagged_parent |= TAG_SPLIT; else (n)->tagged_parent &= ~TAG_SPLIT; } while(0)

struct Workspace {
    Node *tree_root;
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
    unsigned char curtag;
    unsigned char next_split;
    unsigned char ntags;
    unsigned char tags[WORKSPACE_COUNT];
    unsigned char dirty;
};



static void *ecalloc(size_t);
static void arrange(Monitor *);
static void drawbar(Monitor *);
static void focus(Monitor *, Node *);
static void unmanage(Client *);
static Client *find_client(Window);
static Monitor *mon_from_tag(short);
static Node *node_first_leaf(Node *);

/* Handlers bound in keys[] and buttons[] */
static void spawn(const Arg *);
static void quit(const Arg *);
static void killclient(const Arg *);
static void togglefullscreen(const Arg *);
static void togglefloating(const Arg *);
static void rotatecmd(const Arg *);
static void focusworkspace(const Arg *);
static void focusworkspace_step(const Arg *);
static void focuscmd(const Arg *);
static void movecmd(const Arg *);
static void setlayoutcmd(const Arg *);

#define gappx 0
#define borderpx 1
static const char *termcmd[]     = { "st", NULL };
static const char *menucmd[]     = {"/bin/sh", "-c", "[ -x $HOME/dotfiles/scripts/nviciod.sh ] && $HOME/dotfiles/scripts/nviciod.sh || ([ -x $HOME/dotfile/dmenuscript ] && $HOME/dotfile/dmenuscript || ([ -x $HOME/.config/i3/nviciod.sh ] && $HOME/.config/i3/nviciod.sh))", NULL};
/* static const float default_ratio = 0.5f; */
#define border_active   0xFFFFFFUL
#define border_inactive 0x2d2d2dUL
#define bar_bg          0x000000UL
#define bar_fg          0xc2c2b0UL
#define bar_hl          0x685742UL
static signed char tagmap[WORKSPACE_COUNT] = {
    1, 1, 1, 1, 1, 1, 1, 1, 0
};
static const unsigned int MODKEY = Mod1Mask;

/* Autostart command (run once at startup, before the event loop) */
static const char *autostart_cmd[] = {
    "/bin/sh", "-c",
    "[ -x $HOME/.config/i3/xrandr.sh ] && $HOME/.config/i3/xrandr.sh || (command -v xrandr >/dev/null 2>&1 && xrandr --output eDP-1 --mode 1366x768 --rate 60 --pos 0x0 --output HDMI-1 --mode 1920x1080 --rate 60 --pos 1366x0 --primary);"
    "[ -x $HOME/.config/i3/audio-autostart.sh ] && $HOME/.config/i3/audio-autostart.sh;"
    "command -v setxkbmap >/dev/null 2>&1 && setxkbmap -option caps:escape;"
    "command -v xset >/dev/null 2>&1 && xset r rate 180 250",
    NULL
};

enum {
    ACT_NONE = 0,
    ACT_TERM,        /* Alt+w */
    ACT_MENU,        /* Alt+d */
    ACT_KILL,        /* Alt+q */
    ACT_QUIT,        /* Alt+Shift+q */
    ACT_FULLSCREEN,  /* Alt+f */
    ACT_FLOATING,    /* Alt+Shift+f / Alt+sp */
    ACT_ROTATE,      /* Alt+r */
    ACT_SPLIT_V,     /* Alt+b */
    ACT_SPLIT_H,     /* Alt+v */
    ACT_WS_PREV,     /* Alt+a */
    ACT_WS_NEXT,     /* Alt+s */
    ACT_FOCUS_L,     /* Alt+h */
    ACT_FOCUS_R,     /* Alt+l */
    ACT_FOCUS_U,     /* Alt+k */
    ACT_FOCUS_D,     /* Alt+j */
    ACT_MOVE_L,      /* Alt+Shift+h */
    ACT_MOVE_R,      /* Alt+Shift+l */
    ACT_MOVE_U,      /* Alt+Shift+k */
    ACT_MOVE_D       /* Alt+Shift+j */
};

#define KEY_BASE 24
#define KEY_MAX  65
#define KEY_SPAN (KEY_MAX - KEY_BASE + 1)

static const unsigned char key_lut[KEY_SPAN] = {
    [24 - KEY_BASE] = ACT_KILL,
    [25 - KEY_BASE] = ACT_TERM,
    [27 - KEY_BASE] = ACT_ROTATE,
    [38 - KEY_BASE] = ACT_WS_PREV,
    [39 - KEY_BASE] = ACT_WS_NEXT,
    [40 - KEY_BASE] = ACT_MENU,
    [41 - KEY_BASE] = ACT_FULLSCREEN,
    [43 - KEY_BASE] = ACT_FOCUS_L,
    [44 - KEY_BASE] = ACT_FOCUS_D,
    [45 - KEY_BASE] = ACT_FOCUS_U,
    [46 - KEY_BASE] = ACT_FOCUS_R,
    [55 - KEY_BASE] = ACT_SPLIT_H,
    [56 - KEY_BASE] = ACT_SPLIT_V,
    [65 - KEY_BASE] = ACT_FLOATING,
};

static const unsigned char key_shift_lut[KEY_SPAN] = {
    [24 - KEY_BASE] = ACT_QUIT,
    [41 - KEY_BASE] = ACT_FLOATING,
    [43 - KEY_BASE] = ACT_MOVE_L,
    [44 - KEY_BASE] = ACT_MOVE_D,
    [45 - KEY_BASE] = ACT_MOVE_U,
    [46 - KEY_BASE] = ACT_MOVE_R,
};

static const unsigned char grabbed_keys[] = {
    24, 25, 27, 38, 39, 40, 41, 43, 44, 45, 46, 55, 56, 65
};

static const unsigned char mod_combos[] = { 0, LockMask, (1 << 4), (1 << 4) | LockMask };
enum { NODEPOOL = 32 };
#define HEAP_SIZE 512

/* Monolithic Core WM State Arena (Continuous Memory Block) */
typedef struct {
    Workspace workspaces[WORKSPACE_COUNT];
    Node nodepool[NODEPOOL] __attribute__((aligned(16)));
    char heap[HEAP_SIZE];
    unsigned long heap_ptr;
    Window root_win;
    Monitor *mons;
    Monitor *selmon;
    Client *clients;
    Client *client_freelist;
    Node *nodefreelist;
    Atom atoms[ATOM_LAST];
    unsigned char running;
    unsigned char nodeidx;
} WMArena;

static WMArena S;

#define root            (S.root_win)
#define mons            (S.mons)
#define selmon          (S.selmon)
#define workspaces      (S.workspaces)
#define clients         (S.clients)
#define atoms           (S.atoms)
#define running         (S.running)
#define nodeidx         (S.nodeidx)
#define nodepool        (S.nodepool)
#define nodefreelist    (S.nodefreelist)
#define client_freelist (S.client_freelist)
#define heap            (S.heap)
#define heap_ptr        (S.heap_ptr)

static inline Client *selclient(void)
{
    return (selmon && selmon->ws->focus) ? selmon->ws->focus->u.client : NULL;
}

#define free(p) ((void)0)

static void *ecalloc(size_t size)
{
    void *ptr = heap + heap_ptr;
    heap_ptr += (size + 7UL) & ~7UL;
    return ptr;
}

static void try_exec(const char *file, char *const argv[])
{
    char buf[256];

    for (const char *s = file; *s; s++) {
        if (*s == '/') {
            sys_execve(file, argv, envp_global);
            return;
        }
    }

    const char *p = env_path;
    while (*p) {
        char *d = buf;
        while (*p && *p != ':') *d++ = *p++;
        *d++ = '/';
        const char *s = file;
        while (*s) *d++ = *s++;
        *d = '\0';
        sys_execve(buf, argv, envp_global);
        if (*p) p++;
    }
}

void spawn(const Arg *arg)
{
    const char **argv = (const char **)arg->v;
    if (!argv || !argv[0])
        return;
    long pid = sys_fork();
    if (pid == 0) {
        if (sys_fork() == 0) {
            if (dpy)
                sys_close(ConnectionNumber(dpy));
            sys_setsid();
            try_exec(argv[0], (char *const *)argv);
            sys_exit(1);
        }
        sys_exit(0);
    }
    if (pid > 0)
        sys_wait4(pid, NULL, 0, NULL);
}
Node *node_new(void)
{
    Node *n = nodefreelist;
    if (n) {
        nodefreelist = NODE_PARENT(n);
    } else {
        n = &nodepool[nodeidx++];
    }
    __builtin_memset(n, 0, sizeof(Node));
    NODE_SET_LEAF(n, 1);
    return n;
}

static void node_free(Node *n)
{
    if (!n) return;
    if (NODE_IS_LEAF(n) && n->u.client && n->u.client->node == n)
        n->u.client->node = NULL;
    NODE_SET_PARENT(n, nodefreelist);
    nodefreelist = n;
}



Node *node_detach(Node *n, Workspace *ws)
{
    Node *parent, *sibling, *grandparent, *newfocus = NULL;

    if (!n || !NODE_IS_LEAF(n)) return NULL;
    parent = NODE_PARENT(n);
    if (!parent) {
        ws->tree_root = NULL;
    } else {
        sibling   = (parent->u.ch.first == n) ? parent->u.ch.second : parent->u.ch.first;
        grandparent = NODE_PARENT(parent);

        NODE_SET_PARENT(sibling, grandparent);
        if (grandparent) {
            if (grandparent->u.ch.first == parent)
                grandparent->u.ch.first = sibling;
            else
                grandparent->u.ch.second = sibling;
        } else {
            ws->tree_root = sibling;
        }

        node_free(parent);
        newfocus = node_first_leaf(sibling);
    }
    node_free(n);
    ws->focus = newfocus;
    return newfocus;
}

static Node *tree_insert(Workspace *ws, Client *c, int split)
{
    Node *newleaf = node_new();
    newleaf->u.client = c;
    c->node = newleaf;

    Node *focus = (ws->focus) ? ws->focus : (ws->tree_root ? node_first_leaf(ws->tree_root) : NULL);
    if (!focus || !ws->tree_root) {
        ws->tree_root = newleaf;
    } else {
        Node *newnode = node_new();
        NODE_SET_LEAF(newnode, 0);
        NODE_SET_SPLIT(newnode, (split >= 0) ? split : (NODE_PARENT(focus) ? !NODE_SPLIT(NODE_PARENT(focus)) : SPLIT_VERTICAL));
        NODE_SET_PARENT(newnode, NODE_PARENT(focus));
        newnode->u.ch.first  = focus;
        newnode->u.ch.second = newleaf;
        NODE_SET_PARENT(focus, newnode);
        NODE_SET_PARENT(newleaf, newnode);

        if (NODE_PARENT(newnode)) {
            if (NODE_PARENT(newnode)->u.ch.first == focus)
                NODE_PARENT(newnode)->u.ch.first = newnode;
            else
                NODE_PARENT(newnode)->u.ch.second = newnode;
        } else {
            ws->tree_root = newnode;
        }
    }
    ws->focus = newleaf;
    return newleaf;
}

Node *node_insert(Monitor *m, Client *c)
{
    int s = m->next_split;
    m->next_split = -1;
    return tree_insert(m->ws, c, s);
}

Node *node_in_direction(Node *n, int orient, int dir)
{
    Node *p, *c;
    int want_first = (dir == 0);

    if (!n || !NODE_IS_LEAF(n)) return NULL;

    p = n;
    while (NODE_PARENT(p)) {
        Node *parent = NODE_PARENT(p);
        if (NODE_SPLIT(parent) == orient) {
            int on_first = (parent->u.ch.first == p);
            if (on_first != want_first) {
                c = want_first ? parent->u.ch.first : parent->u.ch.second;

                int sub_pos = -1;
                Node *q = n;
                while (q != parent) {
                    Node *qp = NODE_PARENT(q);
                    if (qp && NODE_SPLIT(qp) != orient)
                        sub_pos = (qp->u.ch.first == q) ? 0 : 1;
                    q = qp;
                }

                while (!NODE_IS_LEAF(c)) {
                    if (NODE_SPLIT(c) != orient && sub_pos >= 0)
                        c = sub_pos ? c->u.ch.second : c->u.ch.first;
                    else
                        c = dir ? c->u.ch.first : c->u.ch.second;
                }
                return c;
            }
        }
        p = parent;
    }
    return NULL;
}

Node *node_first_leaf(Node *n)
{
    if (!n) return NULL;
    while (!NODE_IS_LEAF(n)) n = n->u.ch.first;
    return n;
}



static void
arrange_floating(Monitor *m)
{
    Client *c;

    if (!m) return;

    for (c = clients; c; c = c->next) {
        if (!(c->flags & FLAG_FLOATING)) continue;
        if (c->workspace != m->curtag) continue;

        XMoveResizeWindow(dpy, c->win, c->g.r.x, c->g.r.y, c->g.r.w, c->g.r.h);
        XSetWindowBorder(dpy, c->win, (c->node && c->node != m->ws->focus) ? border_inactive : border_active);
    }
}
void arrange_node(Node *n, int x, int y, int w, int h)
{
    Client *c;
    if (!n) return;

    if (NODE_IS_LEAF(n)) {
        c = n->u.client;
        if (!c || (c->flags & (FLAG_FULLSCREEN | FLAG_FLOATING)))
            return;
        unsigned short nx = (unsigned short)(x + borderpx);
        unsigned short ny = (unsigned short)(y + borderpx);
        unsigned short nw = (unsigned short)MAX(1, w - (borderpx << 1));
        unsigned short nh = (unsigned short)MAX(1, h - (borderpx << 1));
        unsigned long ngeom = ((unsigned long)nh << 48) | ((unsigned long)nw << 32) | ((unsigned long)ny << 16) | nx;
        if (c->g.geom_u64 != ngeom) {
            c->g.geom_u64 = ngeom;
            XMoveResizeWindow(dpy, c->win, nx, ny, nw, nh);
        }
        return;
    }

    int half_gap = gappx > 0 ? (gappx >> 1) : 0;
    if (NODE_SPLIT(n) == SPLIT_VERTICAL) {
        int fw = MAX(1, (w >> 1) - half_gap);
        arrange_node(n->u.ch.first,  x, y, fw, h);
        arrange_node(n->u.ch.second, x + fw + gappx, y, MAX(1, w - fw - gappx), h);
    } else {
        int fh = MAX(1, (h >> 1) - half_gap);
        arrange_node(n->u.ch.first,  x, y, w, fh);
        arrange_node(n->u.ch.second, x, y + fh + gappx, w, MAX(1, h - fh - gappx));
    }
}

void arrange(Monitor *m)
{
    int wx, wy, ww, wh;

    if (!m) return;

    if (m->barwin) {
        if (m->ws->fs_client) {
            XUnmapWindow(dpy, m->barwin);
        } else {
            XMapWindow(dpy, m->barwin);
            int bw = m->w - (gappx << 1);
            if (m->bar_width != bw) {
                m->bar_width = bw;
                XMoveResizeWindow(dpy, m->barwin,
                                  m->x + gappx, m->y + gappx,
                                  m->bar_width, BAR_HEIGHT);
            }
        }
    }

    wx = m->x + gappx;
    wy = m->y + gappx + BAR_HEIGHT + gappx;
    ww = m->w - gappx - borderpx;
    wh = m->h - BAR_HEIGHT - (gappx << 1) - borderpx;

    if (m->ws->tree_root)
        arrange_node(m->ws->tree_root, wx, wy, ww, wh);

    arrange_floating(m);

    Client *fs = m->ws->fs_client;
    if (fs) {
        XMoveResizeWindow(dpy, fs->win, m->x, m->y, m->w, m->h);
        XRaiseWindow(dpy, fs->win);
    }

    drawbar(m);
    XFlush(dpy);
}
void manage(Window w, XWindowAttributes *wa)
{
    Client *c;

    if (client_freelist) {
        c = client_freelist;
        client_freelist = c->next;
        c->flags = 0;
    } else {
        c = ecalloc(sizeof(Client));
    }
    c->win = w;
    c->node = NULL;
    c->g.r.x   = wa->x;
    c->g.r.y   = wa->y;
    c->g.r.w   = wa->width;
    c->g.r.h   = wa->height;
    c->workspace = selmon ? selmon->curtag : 0;



    c->next = clients;
    clients = c;

    XSetWindowBorderWidth(dpy, w, borderpx);
    client_set_attributes(dpy, w, bar_bg, border_inactive);

    for (int i = 0; i < 4; i++) {
        XGrabButton(dpy, Button1, MODKEY | mod_combos[i], w);
        XGrabButton(dpy, Button3, MODKEY | mod_combos[i], w);
    }

    XMapWindow(dpy, w);
    if (!(c->flags & FLAG_FLOATING)) {
        Node *nl = node_insert(selmon, c);
        selmon->dirty = 1;
        arrange(selmon);
        if (nl) focus(selmon, nl);
    }
}

void unmanage(Client *c)
{
    Monitor *m;

    if (!c) return;


    m = mon_from_tag(c->workspace);
    if (m) {
        if (m->ws->fs_client == c)
            m->ws->fs_client = NULL;
        if (c->node)
            node_detach(c->node, m->ws);
        m->dirty = 1;
        arrange(m);
    }

    for (Client **pp = &clients; *pp; pp = &(*pp)->next) {
        if (*pp == c) {
            *pp = c->next;
            break;
        }
    }

    c->next = client_freelist;
    c->node = NULL;
    client_freelist = c;
}

static void set_active_focus(Window w)
{
    unsigned int *p = (unsigned int *)_xwire_reserve(dpy->fd, 12);
    p[0] = (3 << 16) | (1 << 8) | 42; /* X_SetInputFocus: revert_to=PointerRoot (1) */
    p[1] = (unsigned int)(w ? w : root);
    p[2] = 0;
    unsigned int win_id = (unsigned int)w;
    XChangeProperty32(dpy, root, atoms[NET_ACTIVE_WINDOW],
                      XA_WINDOW, &win_id, 1);
}

void focus(Monitor *m, Node *n)
{
    if (!m || !n || !n->u.client) return;
    Node *old = m->ws->focus;
    if (old && old->u.client && old != n)
        XSetWindowBorder(dpy, old->u.client->win, border_inactive);
    m->ws->focus = n;
    Window w = n->u.client->win;
    XSetWindowBorder(dpy, w, border_active);
    XRaiseWindow(dpy, w);
    set_active_focus(w);
}

Client *find_client(Window w)
{
    if (__builtin_expect(!w || w == root, 0)) return NULL;
    for (Client *c = clients; c; c = c->next)
        if (c->win == w) return c;
    return NULL;
}

Monitor *mon_from_tag(short tag)
{
    for (Monitor *m = mons; m; m = m->next)
        if (m->curtag == tag) return m;
    return NULL;
}

static void dragmouse(Client *c, const XButtonEvent *bev, int is_resize)
{
    int ox, oy;
    XEvent ev;

    if (!c) return;

    if (!(c->flags & FLAG_FLOATING)) {
        Arg a = {0};
        togglefloating(&a);
    }

    ox = is_resize ? c->g.r.x : bev->x_root - c->g.r.x;
    oy = is_resize ? c->g.r.y : bev->y_root - c->g.r.y;

    if (grab_mouse_motion(dpy) != GrabSuccess)
        return;

    for (;;) {
        XNextEvent(dpy, &ev);
        if (ev.type == MotionNotify) {
            if (is_resize) {
                XResizeWindow(dpy, c->win,
                              MAX(1, ev.xmotion.x_root - ox),
                              MAX(1, ev.xmotion.y_root - oy));
            } else {
                XMoveWindow(dpy, c->win,
                            ev.xmotion.x_root - ox,
                            ev.xmotion.y_root - oy);
            }
            _xwire_flush(dpy->fd);
        } else if (ev.type == ButtonRelease) {
            Monitor *target_mon = selmon;
            if (is_resize) {
                c->g.r.w = MAX(1, ev.xbutton.x_root - ox);
                c->g.r.h = MAX(1, ev.xbutton.y_root - oy);
                for (Monitor *tm = mons; tm; tm = tm->next) {
                    if (c->g.r.x >= tm->x && c->g.r.x < tm->x + tm->w &&
                        c->g.r.y >= tm->y && c->g.r.y < tm->y + tm->h) {
                        target_mon = tm;
                        break;
                    }
                }
                int b2 = borderpx << 1;
                if (c->g.r.x + c->g.r.w + b2 > target_mon->x + target_mon->w)
                    c->g.r.w = target_mon->x + target_mon->w - c->g.r.x - b2;
                if (c->g.r.y + c->g.r.h + b2 > target_mon->y + target_mon->h)
                    c->g.r.h = target_mon->y + target_mon->h - c->g.r.y - b2;
                c->g.r.w = MAX(1, c->g.r.w);
                c->g.r.h = MAX(1, c->g.r.h);
            } else {
                c->g.r.x = ev.xbutton.x_root - ox;
                c->g.r.y = ev.xbutton.y_root - oy;
                int midx = c->g.r.x + (c->g.r.w >> 1);
                int midy = c->g.r.y + (c->g.r.h >> 1);
                for (Monitor *tm = mons; tm; tm = tm->next) {
                    if (midx >= tm->x && midx < tm->x + tm->w &&
                        midy >= tm->y && midy < tm->y + tm->h) {
                        target_mon = tm;
                        break;
                    }
                }
                if (target_mon) {
                    Monitor *old_m = selmon;
                    if (target_mon != selmon) {
                        selmon->dirty = 1;
                        selmon = target_mon;
                        selmon->dirty = 1;
                    }
                    c->workspace = target_mon->curtag;
                    int b2 = borderpx << 1;
                    int max_x = target_mon->x + target_mon->w - c->g.r.w - b2;
                    int max_y = target_mon->y + target_mon->h - c->g.r.h - b2;
                    if (c->g.r.x > max_x) c->g.r.x = max_x;
                    if (c->g.r.y > max_y) c->g.r.y = max_y;
                    if (c->g.r.x < target_mon->x) c->g.r.x = target_mon->x;
                    if (c->g.r.y < target_mon->y) c->g.r.y = target_mon->y;
                    if (old_m != target_mon)
                        arrange(old_m);
                }
            }
            ungrab_mouse(dpy);
            arrange(selmon);
            return;
        }
    }
}

static inline int _xinerama_req_reply(Display *d, unsigned char rep[32]) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 16);
    p[0] = (4 << 16) | 98;
    p[1] = 8;
    *(unsigned long *)&p[2] = 0x414d4152454e4958ULL; /* "XINERAMA" */
    _xwire_flush(d->fd);

    if (_xwire_read(d->fd, rep, 32) != 32 || rep[0] != 1 || !rep[8])
        return 0;

    unsigned int op = rep[9];
    p = (unsigned int *)_xwire_reserve(d->fd, 4);
    *p = (1 << 16) | (5 << 8) | op; /* minor 5: XineramaQueryScreens */
    _xwire_flush(d->fd);
    return (_xwire_read(d->fd, rep, 32) == 32 && rep[0] == 1);
}



static Monitor *
monitor_new(int num, int x, int y, int w, int h)
{
    Monitor *m = ecalloc(sizeof(Monitor));
    m->num = num;
    m->x = x; m->y = y; m->w = w; m->h = h;
    int initial_tag = 0;
    m->ntags = 0;
    for (int t = 0; t < WORKSPACE_COUNT; t++) {
        if (tagmap[t] == num) {
            if (m->ntags == 0) initial_tag = t;
            m->tags[m->ntags++] = t;
        }
    }
    m->curtag = initial_tag;
    m->ws = &workspaces[initial_tag];
    m->bar_width = w;
    m->next_split = -1;
    return m;
}

void updategeom(void)
{
    mons = NULL;

    unsigned char rep[32];
    int nunique = 0;
    unsigned long unique[8];

    if (_xinerama_req_reply(dpy, rep)) {
        unsigned int count = *(unsigned int *)&rep[8];
        for (unsigned int i = 0; i < count && nunique < 16; i++) {
            unsigned char s[8];
            if (_xwire_read(dpy->fd, s, 8) != 8) break;
            unsigned long geom = *(unsigned long *)s;
            int is_unique = 1;
            for (int j = 0; j < nunique; j++)
                if (unique[j] == geom) { is_unique = 0; break; }
            if (is_unique) {
                unique[nunique++] = geom;
                Monitor *m = monitor_new(nunique - 1,
                                         *(short *)&s[0], *(short *)&s[2],
                                         *(unsigned short *)&s[4], *(unsigned short *)&s[6]);
                m->next = mons;
                mons = m;
                if (nunique == 1) selmon = m;
            }
        }
        if (nunique <= 1) {
            for (int t = 0; t < WORKSPACE_COUNT; t++) tagmap[t] = 0;
        }
    }

    if (!mons) {
        for (int t = 0; t < WORKSPACE_COUNT; t++) tagmap[t] = 0;
        mons = monitor_new(0, 0, 0,
                           DisplayWidth(dpy, XDefaultScreen(dpy)),
                           DisplayHeight(dpy, XDefaultScreen(dpy)));
        selmon = mons;
    }
}

static Monitor *
monitor_in_direction(Monitor *m, int orient, int dir)
{
    Monitor *target = NULL, *tm;

    for (tm = mons; tm; tm = tm->next) {
        if (tm == m) continue;
        int m_pos = orient == SPLIT_VERTICAL ? m->x : m->y;
        int m_dim = orient == SPLIT_VERTICAL ? m->w : m->h;
        int tm_pos = orient == SPLIT_VERTICAL ? tm->x : tm->y;
        int tm_dim = orient == SPLIT_VERTICAL ? tm->w : tm->h;
        int m_ortho = orient == SPLIT_VERTICAL ? m->y : m->x;
        int m_ortho_dim = orient == SPLIT_VERTICAL ? m->h : m->w;
        int tm_ortho = orient == SPLIT_VERTICAL ? tm->y : tm->x;
        int tm_ortho_dim = orient == SPLIT_VERTICAL ? tm->h : tm->w;

        if (tm_ortho >= m_ortho + m_ortho_dim || tm_ortho + tm_ortho_dim <= m_ortho)
            continue;
        if (dir == 0) {
            if (tm_pos + tm_dim <= m_pos && (!target || tm_pos + tm_dim > (orient == SPLIT_VERTICAL ? target->x + target->w : target->y + target->h)))
                target = tm;
        } else {
            if (tm_pos >= m_pos + m_dim && (!target || tm_pos < (orient == SPLIT_VERTICAL ? target->x : target->y)))
                target = tm;
        }
    }
    return target;
}



void focusworkspace(const Arg *arg)
{
    Monitor *m, *prev;
    int tag = arg->i;

    if (tag < 0 || tag >= WORKSPACE_COUNT || !selmon)
        return;

    prev = selmon;
    m = selmon;

    /* Redirect to pinned monitor if this workspace has a home */
    if (tagmap[tag] >= 0) {
        for (Monitor *tm = mons; tm; tm = tm->next) {
            if (tm->num == tagmap[tag]) {
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
        goto done;
    }

    int oldtag = m->curtag;

    m->curtag = tag;
    m->ws     = &workspaces[tag];
    m->dirty  = 1;

    Node *f = (m->ws->focus && m->ws->focus->u.client) ? m->ws->focus : (m->ws->tree_root ? node_first_leaf(m->ws->tree_root) : NULL);
    if (f)
        focus(m, f);
    else
        set_active_focus(0);

    for (Client *fc = clients; fc; fc = fc->next) {
        if (fc->workspace == oldtag) {
            int vis = 0;
            for (Monitor *tm = mons; tm; tm = tm->next)
                if (tm != m && tm->curtag == oldtag) { vis = 1; break; }
            if (!vis) hide_window(dpy, fc->win);
        }
    }

done:
    if (prev != m) { prev->dirty = 1; m->dirty = 1; arrange(prev); }
    arrange(m);
}

void focusworkspace_step(const Arg *arg)
{
    if (!selmon) return;
    int tag = (selmon->curtag + arg->i + WORKSPACE_COUNT) % WORKSPACE_COUNT;
    Arg a = {.i = tag};
    focusworkspace(&a);
}
void drawbar(Monitor *m)
{
    int i, x, sw, idx;
    Drawable d;

    if (!m || !m->barwin) return;
    if (!m->dirty) return;
    m->dirty = 0;

    if (m->ntags == 0) return;

    sw = m->bar_width / m->ntags;

    d = m->barpm;

    XSetForeground(dpy, bar_bg);
    XFillRectangle(dpy, d, 0, 0, m->bar_width, BAR_HEIGHT);

    unsigned long cur_fg = bar_bg;
    int half_sw = (sw >> 1) - 4;
    int ypos = (BAR_HEIGHT >> 1) + 5;
    for (idx = 0, x = 0; idx < m->ntags; idx++, x += sw) {
        i = m->tags[idx];
        char ch = (char)('1' + i);
        unsigned long fg = (m == selmon && m->curtag == i) ? bar_fg : bar_hl;
        if (fg != cur_fg) {
            XSetForeground(dpy, fg);
            cur_fg = fg;
        }
        XDrawChar(dpy, d, x + half_sw, ypos, ch);
    }

    if (m->barpm)
        XCopyArea(dpy, m->barpm, m->barwin, m->bar_width, BAR_HEIGHT);
}

void createbars(void)
{
    for (Monitor *m = mons; m; m = m->next) {
        m->bar_width = m->w - (gappx << 1);
        unsigned int wid = dpy->next_rid++;
        unsigned int *p = (unsigned int *)_xwire_reserve(dpy->fd, 40);
        p[0] = (10 << 16) | 1; /* Opcode 1: CreateWindow, depth=0, length=10 words */
        p[1] = wid;
        p[2] = (unsigned int)root;
        p[3] = ((int)(m->y + gappx) << 16) | ((int)(m->x + gappx) & 0xFFFF);
        p[4] = ((unsigned int)BAR_HEIGHT << 16) | (m->bar_width & 0xFFFF);
        p[5] = 1 << 16;        /* border_width=0, class=InputOutput (1) */
        p[6] = 0;              /* visual = CopyFromParent */
        p[7] = CWOverrideRedirect | CWEventMask;
        p[8] = 1;              /* override_redirect = True */
        p[9] = ExposureMask | ButtonPressMask;
        m->barwin = wid;
        m->barpm = XCreatePixmap(dpy, root, m->w, BAR_HEIGHT, 24);
        if (!bar_gc) bar_gc = XCreateGC(dpy, wid);

        XMapWindow(dpy, m->barwin);
        XLowerWindow(dpy, m->barwin);
    }
}
void keypress(XEvent *e)
{
    unsigned int kc = e->xkey.keycode;
    unsigned int state = CLEANMASK(e->xkey.state);

    if (kc >= 10 && kc <= 18) {
        Arg a = {.i = (int)(kc - 10)};
        if (state == MODKEY) {
            focusworkspace(&a);
            return;
        }
        if (state == (MODKEY | ShiftMask)) {
            movecmd(&a);
            return;
        }
    }

    if (kc >= KEY_BASE && kc <= KEY_MAX) {
        unsigned char act = 0;
        if (state == MODKEY)
            act = key_lut[kc - KEY_BASE];
        else if (state == (MODKEY | ShiftMask))
            act = key_shift_lut[kc - KEY_BASE];

        switch (act) {
        case ACT_TERM:        { Arg a = {.v = termcmd}; spawn(&a); } break;
        case ACT_MENU:        { Arg a = {.v = menucmd}; spawn(&a); } break;
        case ACT_KILL:        killclient(0); break;
        case ACT_QUIT:        quit(0); break;
        case ACT_FULLSCREEN:  togglefullscreen(0); break;
        case ACT_FLOATING:    togglefloating(0); break;
        case ACT_ROTATE:      rotatecmd(0); break;
        case ACT_SPLIT_V:     { Arg a = {.i = SPLIT_VERTICAL}; setlayoutcmd(&a); } break;
        case ACT_SPLIT_H:     { Arg a = {.i = SPLIT_HORIZONTAL}; setlayoutcmd(&a); } break;
        case ACT_WS_PREV:     { Arg a = {.i = -1}; focusworkspace_step(&a); } break;
        case ACT_WS_NEXT:     { Arg a = {.i = +1}; focusworkspace_step(&a); } break;
        case ACT_FOCUS_L:     { Arg a = {.i = (SPLIT_VERTICAL<<1)|0}; focuscmd(&a); } break;
        case ACT_FOCUS_R:     { Arg a = {.i = (SPLIT_VERTICAL<<1)|1}; focuscmd(&a); } break;
        case ACT_FOCUS_U:     { Arg a = {.i = (SPLIT_HORIZONTAL<<1)|0}; focuscmd(&a); } break;
        case ACT_FOCUS_D:     { Arg a = {.i = (SPLIT_HORIZONTAL<<1)|1}; focuscmd(&a); } break;
        case ACT_MOVE_L:      { Arg a = {.i = 100 + ((SPLIT_VERTICAL<<1)|0)}; movecmd(&a); } break;
        case ACT_MOVE_R:      { Arg a = {.i = 100 + ((SPLIT_VERTICAL<<1)|1)}; movecmd(&a); } break;
        case ACT_MOVE_U:      { Arg a = {.i = 100 + ((SPLIT_HORIZONTAL<<1)|0)}; movecmd(&a); } break;
        case ACT_MOVE_D:      { Arg a = {.i = 100 + ((SPLIT_HORIZONTAL<<1)|1)}; movecmd(&a); } break;
        }
    }
}

void buttonpress(XEvent *e)
{
    Client *c = find_client(e->xbutton.window);
    if (!c && e->xbutton.subwindow)
        c = find_client(e->xbutton.subwindow);
    Monitor *m;

    for (m = mons; m; m = m->next) {
        if (e->xbutton.window == m->barwin || e->xbutton.subwindow == m->barwin) {
            if (m->ntags > 0) {
                int sw = m->w / m->ntags;
                int xpos = e->xbutton.x_root - m->x;
                if (xpos < 0) xpos = 0;
                int clicked = xpos / sw;
                if (clicked >= m->ntags) clicked = m->ntags - 1;
                Arg a = {.i = m->tags[clicked]};
                focusworkspace(&a);
            }
            return;
        }
    }

    if (c && (CLEANMASK(e->xbutton.state) & MODKEY)) {
        for (m = mons; m; m = m->next) {
            if (c->workspace == m->curtag ||
                (e->xbutton.x_root >= m->x && e->xbutton.x_root < m->x + m->w &&
                 e->xbutton.y_root >= m->y && e->xbutton.y_root < m->y + m->h)) {
                selmon = m;
                break;
            }
        }
        if (c->node)
            focus(selmon, c->node);

        dragmouse(c, &e->xbutton, e->xbutton.button == Button3);
        return;
    }

    if (c && c->node) {
        m = mon_from_tag(c->workspace);
        if (m && m->ws->focus != c->node) {
            focus(m, c->node);
            arrange(m);
            return;
        }
    }
}

void enternotify(XEvent *e)
{
    XCrossingEvent *ev = &e->xcrossing;
    Monitor *m;
    Client *c;
    Node *n;

    if (ev->mode != NotifyNormal || ev->detail == NotifyInferior)
        return;
    c = find_client(ev->window);
    if (!c || !c->node) return;

    n = c->node;
    m = mon_from_tag(c->workspace);
    if (!m) return;

    if (m != selmon) {
        Monitor *prev = selmon;
        selmon = m;
        if (prev) { prev->dirty = 1; drawbar(prev); }
        m->dirty = 1;
    }
    if (n != m->ws->focus) {
        focus(m, n);
        m->dirty = 1;
    }
    drawbar(m);
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

void configurerequest(XEvent *e)
{
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    Client *c = find_client(ev->window);

    if (c) {
        if (!(c->flags & FLAG_FLOATING)) return;
        c->g.r.x = ev->vals[0]; c->g.r.y = ev->vals[1];
        c->g.r.w = ev->vals[2]; c->g.r.h = ev->vals[3];
    }
    XConfigureWindowFromReq(dpy, ev);
}
void quit(const Arg *arg)
{
    running = 0;
}

void killclient(const Arg *arg)
{
    Client *c = selclient();
    if (!c) return;

    unsigned int *p = (unsigned int *)_xwire_reserve(dpy->fd, 44);
    p[0] = (11 << 16) | 25; /* X_SendEvent: reqType=25, length=11, propagate=0 */
    p[1] = (unsigned int)c->win;
    p[2] = 0;               /* mask = NoEventMask */
    p[3] = (32 << 8) | 33;  /* ClientMessage (33), format=32 */
    p[4] = (unsigned int)c->win;
    p[5] = (unsigned int)atoms[WM_PROTOCOLS];
    p[6] = (unsigned int)atoms[WM_DELETE_WINDOW];
    p[7] = 0;               /* CurrentTime */
    p[8] = 0;
    p[9] = 0;
    p[10] = 0;
}

void togglefullscreen(const Arg *arg)
{
    Client *c = selclient();
    if (!c) return;

    c->flags ^= FLAG_FULLSCREEN;
    selmon->ws->fs_client = (c->flags & FLAG_FULLSCREEN) ? c : NULL;
    XSetWindowBorderWidth(dpy, c->win, selmon->ws->fs_client ? 0 : borderpx);
    selmon->dirty = 1;
    arrange(selmon);
}

void togglefloating(const Arg *arg)
{
    Client *c = selclient();
    int fw, fh;
    if (!c) return;

    c->flags ^= FLAG_FLOATING;
    if (c->flags & FLAG_FLOATING) {
        fw = ((unsigned int)selmon->w * 39322U) >> 16;
        fh = ((unsigned int)selmon->h * 39322U) >> 16;
        c->g.r.x = selmon->x + ((selmon->w - fw) >> 1);
        c->g.r.y = selmon->y + ((selmon->h - fh) >> 1);
        c->g.r.w = fw; c->g.r.h = fh;
        c->workspace = selmon->curtag;
        XRaiseWindow(dpy, c->win);
    } else {
        if (!c->node)
            node_insert(selmon, c);
    }
    arrange(selmon);
}

void rotatecmd(const Arg *arg)
{
    if (selmon && selmon->ws->focus && NODE_PARENT(selmon->ws->focus)) {
        Node *p = NODE_PARENT(selmon->ws->focus);
        Node *tmp = p->u.ch.first;
        p->u.ch.first  = p->u.ch.second;
        p->u.ch.second = tmp;
        arrange(selmon);
    }
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
            if ((!n || !n->u.client) && om->ws->tree_root)
                n = node_first_leaf(om->ws->tree_root);
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

        c->workspace = tag;
        tree_insert(&workspaces[tag], c, SPLIT_VERTICAL);
        Monitor *target_m = mon_from_tag(tag);
        if (target_m) {
            arrange(target_m);
        } else {
            hide_window(dpy, c->win);
        }
        if (!newfocus) {
            set_active_focus(0);
        }
        arrange(selmon);
        return;
    }

    raw -= 100;
    orient = raw >> 1;
    dir    = raw & 1;

    target = node_in_direction(n, orient, dir);
    if (target) {
        tmpc = n->u.client;
        n->u.client = target->u.client;
        target->u.client = tmpc;
        n->u.client->node = n;
        target->u.client->node = target;
        focus(selmon, target);
        arrange(selmon);
        return;
    }

    {
        Monitor *om = monitor_in_direction(selmon, orient, dir);
        if (om) {
            Client *c = n->u.client;

            node_detach(n, ws);

            Node *nl = tree_insert(om->ws, c, orient);

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
    unsigned int *p = (unsigned int *)_xwire_reserve(dpy->fd, 12);
    p[0] = (3 << 16) | 34; /* X_UngrabKey: reqType=34, length=3, AnyKey=0 */
    p[1] = (unsigned int)root;
    p[2] = 0x8000;         /* AnyModifier */

    for (unsigned int j = 0; j < LENGTH(mod_combos); j++) {
        for (unsigned int i = 0; i < LENGTH(grabbed_keys); i++) {
            XGrabKey(dpy, grabbed_keys[i], MODKEY | mod_combos[j]);
            XGrabKey(dpy, grabbed_keys[i], MODKEY | ShiftMask | mod_combos[j]);
        }
        for (int k = 10; k <= 18; k++) {
            XGrabKey(dpy, k, MODKEY | mod_combos[j]);
            XGrabKey(dpy, k, MODKEY | ShiftMask | mod_combos[j]);
        }
    }
}

void setup(void)
{
    /* Ignore SIGCHLD and set SA_NOCLDWAIT to prevent any child zombies */
    struct {
        void *sa_handler;
        unsigned long sa_flags;
        void *sa_restorer;
        unsigned long sa_mask;
    } sa;
    sa.sa_handler = (void *)1; /* SIG_IGN */
    sa.sa_flags = 2 | 1;       /* SA_NOCLDWAIT (2) | SA_NOCLDSTOP (1) */
    sa.sa_restorer = NULL;
    sa.sa_mask = 0;
    syscall4(SYS_RT_SIGACTION, 17, (long)&sa, 0, 8);

    dpy = XOpenDisplay();
    if (!dpy) sys_exit(1);
    running = 1;
    root = RootWindow(dpy, XDefaultScreen(dpy));


    static const char atom_names[] =
        "WM_PROTOCOLS\0" "WM_DELETE_WINDOW\0"
        "_NET_SUPPORTED\0" "_NET_NUMBER_OF_DESKTOPS\0"
        "_NET_WM_STATE\0" "_NET_WM_STATE_FULLSCREEN\0" "_NET_ACTIVE_WINDOW\0";
    const char *an = atom_names;

    /* Batch pipeline: send all 7 X_InternAtom requests in a single write */
    for (int i = 0; i < ATOM_LAST; i++) {
        int nbytes = 0;
        while (an[nbytes]) nbytes++;
        int words = (11 + nbytes) >> 2;
        unsigned int *p = (unsigned int *)_xwire_reserve(dpy->fd, words << 2);
        p[0] = (words << 16) | 16; /* X_InternAtom */
        p[1] = (unsigned int)nbytes;
        p[words - 1] = 0;
        __builtin_memcpy(&p[2], an, nbytes);
        an += nbytes + 1;
    }
    _xwire_flush(dpy->fd);

    /* Read all 7 replies (32 bytes each) in a single read burst */
    for (int i = 0; i < ATOM_LAST; i++) {
        unsigned char rep[32];
        if (_xwire_read(dpy->fd, rep, 32) == 32 && rep[0] == 1)
            atoms[i] = (Atom)*(unsigned int *)&rep[8];
    }

    XChangeProperty32(dpy, root,
        atoms[NET_SUPPORTED],
        XA_ATOM, &atoms[NET_WM_STATE], 3);

    {
        unsigned int nd = WORKSPACE_COUNT;
        XChangeProperty32(dpy, root,
            atoms[NET_NUMBER_OF_DESKTOPS],
            XA_CARDINAL, &nd, 1);
    }

    XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask |
                            EnterWindowMask | ButtonPressMask |
                            StructureNotifyMask | PropertyChangeMask);

    /* Run autostart (xrandr, etc.) before probing screen geometries */
    Arg a = { .v = autostart_cmd };
    spawn(&a);

    updategeom();
    createbars();
    grabkeys();

    selmon = mons;
    for (Monitor *m = mons; m; m = m->next)
        arrange(m);

    XFlush(dpy);
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
        case UnmapNotify:
        case DestroyNotify:
            {
                Client *c = find_client(ev.xunmap.window);
                if (c) unmanage(c);
            }
            break;
        case ConfigureRequest: configurerequest(&ev); break;
        case Expose:
            for (Monitor *m = mons; m; m = m->next)
                if (m->barwin == ev.xexpose.window) {
                    m->dirty = 1;
                    drawbar(m);
                    break;
                }
            break;
        }
    }
}

void cleanup(void)
{
    XCloseDisplay(dpy);

    if (xorg_pid > 0) {
        sys_kill(xorg_pid, 15);
        long status;
        syscall4(SYS_WAIT4, xorg_pid, (long)&status, 0, 0);
        xorg_pid = 0;
    }
}

static char *custom_envp[48];
static char display_env[] = "DISPLAY=:0";

static void setup_environ(char **envp)
{
    int has_display = 0, count = 0;
    if (envp) {
        for (char **ep = envp; *ep && count < 45; ep++) {
            if (*(const unsigned long *)*ep == 0x3d59414c50534944ULL) {
                has_display = 1;
                env_display = *ep + 8;
            } else if (*(const unsigned int *)*ep == 0x48544150 && (*ep)[4] == '=') {
                env_path = *ep + 5;
            }
            custom_envp[count++] = *ep;
        }
    }
    if (!has_display) custom_envp[count++] = display_env;
    envp_global = custom_envp;
}

__attribute__((used, noinline))
int wm_main(int argc, char **argv, char **envp)
{
    (void)argc; (void)argv;
    setup_environ(envp);
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
