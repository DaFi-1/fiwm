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

static inline long sys_wait4(long pid, int *wstatus, int options, void *rusage)
{
    return syscall4(SYS_WAIT4, pid, (long)wstatus, (long)options, (long)rusage);
}

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
    long pad[24];
} XEvent;


typedef struct {
    int fd;
    Window root;
    int width, height;
    unsigned int rid_base;
    unsigned int rid_mask;
    unsigned int next_rid;
} Display;

/* Syscall wrappers */
static char _xwire_outbuf[4096];
static unsigned long _xwire_outlen = 0;
static unsigned char _xwire_inbuf[1024];
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
static long xorg_pid = 0;

static void start_xorg(void)
{
    char vt_arg[16];
    vt_arg[0] = '\0';
    char tty_path[32];
    long len = syscall3(SYS_READLINK, (long)"/proc/self/fd/0", (long)tty_path, sizeof(tty_path) - 1);
    if (len > 0) {
        tty_path[len] = '\0';
        if (tty_path[0] == '/' && tty_path[5] == 't' && tty_path[6] == 't' && tty_path[7] == 'y' &&
            tty_path[8] >= '0' && tty_path[8] <= '9') {
            vt_arg[0] = 'v';
            vt_arg[1] = 't';
            int p = 8, q = 2;
            while (tty_path[p] >= '0' && tty_path[p] <= '9' && q < 14)
                vt_arg[q++] = tty_path[p++];
            vt_arg[q] = '\0';
        }
    }

    const char *x_argv[6];
    int argc = 0;
    x_argv[argc++] = "/usr/bin/X";
    x_argv[argc++] = ":0";
    x_argv[argc++] = "-ac";
    if (vt_arg[0])
        x_argv[argc++] = vt_arg;
    x_argv[argc++] = "-keeptty";
    x_argv[argc] = NULL;

    long pid = sys_fork();
    if (pid == 0) {
        sys_execve(x_argv[0], (char *const *)x_argv, envp_global);
        sys_exit(1);
    }
    if (pid > 0) {
        xorg_pid = pid;
    }
}

static inline Display *XOpenDisplay(const char *name) {
    int fd = (int)_xwire_sys3(SYS_SOCKET, AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return (void *)0;

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    char path[64] = "/tmp/.X11-unix/X0";

    const char *dstr = name ? name : env_display;

    if (dstr && dstr[0] == ':') {
        int i = 0, j = 16;
        while (dstr[++i] && dstr[i] != '.' && j < 60)
            path[j++] = dstr[i];
        path[j] = '\0';
    }

    for (unsigned long k = 0; k < sizeof(path); k++) addr.sun_path[k] = path[k];

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

#define ConnectionNumber(d) ((d)->fd)
#define XDefaultScreen(d) 0
#define DisplayWidth(d, s) ((d) ? (d)->width : 1024)
#define DisplayHeight(d, s) ((d) ? (d)->height : 768)
#define RootWindow(d, s) ((d) ? (d)->root : 0)
#define DefaultColormap(d, s) 0
#define DefaultDepth(d, s) 24

static inline Window XCreateWindow(Display *d, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width, int depth, unsigned int class_type, VisualID visual, unsigned long valmask, XSetWindowAttributes *a) {
    (void)class_type; (void)visual;
    unsigned int wid = d->next_rid++;
    if (__builtin_expect(_xwire_outlen + 64 > sizeof(_xwire_outbuf), 0))
        _xwire_flush(d->fd);
    unsigned int *p = (unsigned int *)(_xwire_outbuf + _xwire_outlen);
    p[1] = wid;
    p[2] = (unsigned int)parent;
    p[3] = ((int)y << 16) | ((int)x & 0xFFFF);
    p[4] = (height << 16) | (width & 0xFFFF);
    p[5] = (1 << 16) | (border_width & 0xFFFF);
    p[6] = 0;
    p[7] = (unsigned int)valmask;
    int idx = 8;
    if (valmask & (1 << 1)) p[idx++] = (unsigned int)a->background_pixel;
    if (valmask & (1 << 3)) p[idx++] = (unsigned int)a->border_pixel;
    if (valmask & (1 << 9)) p[idx++] = (unsigned int)a->override_redirect;
    if (valmask & (1 << 11)) p[idx++] = (unsigned int)a->event_mask;
    p[0] = (idx << 16) | (depth << 8) | 1;
    _xwire_outlen += (unsigned long)(idx << 2);
    return wid;
}

static inline void XLowerWindow(Display *d, Window w) {
    /* Opcode 12: ConfigureWindow (CWStackMode = 64, Below = 1) */
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 16);
    p[0] = (4 << 16) | 12;
    p[1] = (unsigned int)w;
    p[2] = 64;
    p[3] = 1;
}

static inline void XConfigureWindowFromReq(Display *d, XConfigureRequestEvent *ev) {
    if (__builtin_expect(_xwire_outlen + 40 > sizeof(_xwire_outbuf), 0))
        _xwire_flush(d->fd);
    unsigned int *p = (unsigned int *)(_xwire_outbuf + _xwire_outlen);
    int idx = 3;
    unsigned long valmask = ev->value_mask;
    p[1] = (unsigned int)ev->window;
    p[2] = (unsigned int)valmask;
    if (valmask & 1) p[idx++] = (unsigned int)ev->x;
    if (valmask & 2) p[idx++] = (unsigned int)ev->y;
    if (valmask & 4) p[idx++] = (unsigned int)ev->width;
    if (valmask & 8) p[idx++] = (unsigned int)ev->height;
    if (valmask & 16) p[idx++] = (unsigned int)ev->border_width;
    if (valmask & 32) p[idx++] = (unsigned int)ev->above;
    if (valmask & 64) p[idx++] = (unsigned int)ev->detail;
    p[0] = (idx << 16) | 12;
    _xwire_outlen += (unsigned long)(idx << 2);
}

static inline void XSelectInput(Display *d, Window w, long mask) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 16);
    p[0] = (4 << 16) | 2;
    p[1] = (unsigned int)w;
    p[2] = 0x800;
    p[3] = (unsigned int)mask;
}

static inline void XChangeWindowAttributes(Display *d, Window w, unsigned long valmask, XSetWindowAttributes *a) {
    if (__builtin_expect(_xwire_outlen + 32 > sizeof(_xwire_outbuf), 0))
        _xwire_flush(d->fd);
    unsigned int *p = (unsigned int *)(_xwire_outbuf + _xwire_outlen);
    int idx = 3;
    p[1] = (unsigned int)w;
    p[2] = (unsigned int)valmask;
    if (valmask & (1 << 1)) p[idx++] = (unsigned int)a->background_pixel;
    if (valmask & (1 << 3)) p[idx++] = (unsigned int)a->border_pixel;
    if (valmask & (1 << 11)) p[idx++] = (unsigned int)a->event_mask;
    p[0] = (idx << 16) | 2;
    _xwire_outlen += (unsigned long)(idx << 2);
}

static inline void XSetWindowBorderWidth(Display *d, Window w, unsigned int bw) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 16);
    p[0] = (4 << 16) | 12;
    p[1] = (unsigned int)w;
    p[2] = 16;
    p[3] = bw;
}

static inline void XSetWindowBorder(Display *d, Window w, unsigned long pixel) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 16);
    p[0] = (4 << 16) | 2;
    p[1] = (unsigned int)w;
    p[2] = 8;
    p[3] = (unsigned int)pixel;
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

static inline void XMoveWindow(Display *d, Window w, int x, int y) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 20);
    p[0] = (5 << 16) | 12;
    p[1] = (unsigned int)w;
    p[2] = 3;
    p[3] = (unsigned int)x;
    p[4] = (unsigned int)y;
}

static inline void XResizeWindow(Display *d, Window w, unsigned int width, unsigned int height) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 20);
    p[0] = (5 << 16) | 12;
    p[1] = (unsigned int)w;
    p[2] = 12;
    p[3] = width;
    p[4] = height;
}

static inline void XMapWindow(Display *d, Window w) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 8);
    p[0] = (2 << 16) | 8;
    p[1] = (unsigned int)w;
}

static inline void XUnmapWindow(Display *d, Window w) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 8);
    p[0] = (2 << 16) | 10;
    p[1] = (unsigned int)w;
}

static inline void XRaiseWindow(Display *d, Window w) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 16);
    p[0] = (4 << 16) | 12;
    p[1] = (unsigned int)w;
    p[2] = 64;
    p[3] = 0;
}

static inline void XSetInputFocus(Display *d, Window w, int revert, Time t) {
    (void)revert; (void)t;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 12);
    p[0] = (3 << 16) | (1 << 8) | 42;
    p[1] = (unsigned int)w;
    p[2] = 0;
}

static inline void XGrabKey(Display *d, int keycode, unsigned int mod, Window w, int owner, int sync_p, int sync_k) {
    (void)owner; (void)sync_p; (void)sync_k;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 16);
    p[0] = (4 << 16) | (1 << 8) | 33; /* X_GrabKey: reqType=33, ownerEvents=1, length=4 */
    p[1] = (unsigned int)w;
    p[2] = (1 << 24) | (((unsigned int)(keycode & 0xFF)) << 16) | (mod & 0xFFFF);
    p[3] = 1;
}

static inline void XUngrabKey(Display *d, int keycode, unsigned int mod, Window w) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 12);
    p[0] = (3 << 16) | ((keycode & 0xFF) << 8) | 34; /* X_UngrabKey: reqType=34, key=keycode, length=3 */
    p[1] = (unsigned int)w;
    p[2] = mod & 0xFFFF;
}

static inline void XGrabButton(Display *d, unsigned int btn, unsigned int mod, Window w, int owner, unsigned int mask, int sync_p, int sync_k, Window confine, unsigned long cursor) {
    (void)owner; (void)sync_p; (void)sync_k; (void)confine; (void)cursor;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 24);
    p[0] = (6 << 16) | 28; /* X_GrabButton: reqType=28, ownerEvents=0, length=6 */
    p[1] = (unsigned int)w;
    p[2] = (1 << 24) | (1 << 16) | (mask & 0xFFFF);
    p[3] = (unsigned int)confine;
    p[4] = (unsigned int)cursor;
    p[5] = ((mod & 0xFFFF) << 16) | (btn & 0xFF);
}

static inline void XUngrabButton(Display *d, unsigned int btn, unsigned int mod, Window w) {
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 12);
    p[0] = (3 << 16) | (btn << 8) | 29;
    p[1] = (unsigned int)w;
    p[2] = mod;
}

static inline void XFlush(Display *d) {
    if (d) _xwire_flush(d->fd);
}
static inline void XSync(Display *d, int discard) { (void)discard; if (d) _xwire_flush(d->fd); }

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
        case 3: /* KeyRelease */
        case 4: /* ButtonPress */
        case 5: /* ButtonRelease */
        case 6: /* MotionNotify */
            e->xkey.window = *(const unsigned int *)&buf[12];
            e->xkey.root = *(const unsigned int *)&buf[8];
            e->xkey.subwindow = *(const unsigned int *)&buf[16];
            e->xkey.time = *(const unsigned int *)&buf[4];
            e->xkey.x_root = *(const short *)&buf[20];
            e->xkey.y_root = *(const short *)&buf[22];
            e->xkey.x = *(const short *)&buf[24];
            e->xkey.y = *(const short *)&buf[26];
            e->xkey.keycode = buf[1];
            e->xkey.state = *(const unsigned short *)&buf[28];
            return 0;
        case 20: /* MapRequest */
            e->xmaprequest.window = *(const unsigned int *)&buf[8];
            e->xmaprequest.parent = *(const unsigned int *)&buf[4];
            return 0;
        case 23: /* ConfigureRequest */
            e->xconfigurerequest.window = *(const unsigned int *)&buf[8];
            e->xconfigurerequest.x = *(const short *)&buf[16];
            e->xconfigurerequest.y = *(const short *)&buf[18];
            e->xconfigurerequest.width = *(const unsigned short *)&buf[20];
            e->xconfigurerequest.height = *(const unsigned short *)&buf[22];
            e->xconfigurerequest.border_width = *(const unsigned short *)&buf[24];
            e->xconfigurerequest.value_mask = *(const unsigned short *)&buf[28];
            return 0;
        case 17: /* DestroyNotify */
        case 18: /* UnmapNotify */
            e->xunmap.window = *(const unsigned int *)&buf[8];
            e->xunmap.send_event = 0;
            return 0;
        case 12: /* Expose */
            e->xexpose.window = *(const unsigned int *)&buf[4];
            return 0;
        case 7: /* EnterNotify */
            e->xcrossing.window = *(const unsigned int *)&buf[12];
            e->xcrossing.root = *(const unsigned int *)&buf[8];
            e->xcrossing.subwindow = *(const unsigned int *)&buf[16];
            e->xcrossing.x_root = *(const short *)&buf[20];
            e->xcrossing.y_root = *(const short *)&buf[22];
            e->xcrossing.x = *(const short *)&buf[24];
            e->xcrossing.y = *(const short *)&buf[26];
            e->xcrossing.mode = (int)buf[30];
            e->xcrossing.detail = (int)buf[1];
            return 0;
        default:
            return 0;
        }
    }
}

#define XFree(p) ((void)0)

static inline Atom intern_atom(Display *d, const char *name) {
    if (!d || !name) return 0;
    int nbytes = 0;
    while (name[nbytes]) nbytes++;

    int words = (11 + nbytes) >> 2;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, words << 2);
    p[0] = (words << 16) | 16; /* X_InternAtom */
    p[1] = (unsigned int)nbytes;
    p[words - 1] = 0;

    unsigned char *bp = (unsigned char *)&p[2];
    for (int i = 0; i < nbytes; i++) bp[i] = (unsigned char)name[i];
    _xwire_flush(d->fd);

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
#define XInternAtom(d, name, b) intern_atom(d, name)

static inline unsigned long parse_color(const char *name) {
    unsigned long val = 0;
    if (name && *name == '#') {
        while (*++name) {
            unsigned char c = (unsigned char)*name;
            val = (val << 4) | ((c <= '9') ? (c - '0') : ((c | 0x20) - 'a' + 10));
        }
    }
    return val;
}

static inline GC XCreateGC(Display *d, Drawable dr, unsigned long valmask, void *val) {
    (void)valmask; (void)val;
    if (!d || !dr) return (GC)0;
    unsigned int gcid = d->next_rid++;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 16);
    p[0] = (4 << 16) | 55; /* X_CreateGC: reqType=55, length=4 */
    p[1] = gcid;
    p[2] = (unsigned int)dr;
    p[3] = 0; /* mask = 0 */
    return (GC)(unsigned long)gcid;
}

static inline void XFreeGC(Display *d, GC gc) {
    if (!d || !gc) return;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 8);
    p[0] = (2 << 16) | 60; /* X_FreeGC: reqType=60, length=2 */
    p[1] = (unsigned int)(unsigned long)gc;
}

static inline void XSetForeground(Display *d, GC gc, unsigned long pixel) {
    if (!d || !gc) return;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 16);
    p[0] = (4 << 16) | 56; /* X_ChangeGC: reqType=56, length=4 */
    p[1] = (unsigned int)(unsigned long)gc;
    p[2] = 4; /* GCForeground = (1L<<2) */
    p[3] = (unsigned int)pixel;
}

static inline void XFillRectangle(Display *d, Drawable dr, GC gc, int x, int y, unsigned int w, unsigned int h) {
    if (!d || !gc || !dr) return;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 20);
    p[0] = (5 << 16) | 70; /* X_PolyFillRectangle: reqType=70, length=5 */
    p[1] = (unsigned int)dr;
    p[2] = (unsigned int)(unsigned long)gc;
    p[3] = ((int)y << 16) | ((int)x & 0xFFFF);
    p[4] = ((unsigned int)h << 16) | (w & 0xFFFF);
}

static inline void XDrawString(Display *d, Drawable dr, GC gc, int x, int y, const char *s, int len) {
    if (!d || !dr || !gc || !s) return;
    int words = (21 + len) >> 2;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, words << 2);
    p[0] = (words << 16) | 74; /* X_PolyText8: reqType=74 */
    p[1] = (unsigned int)dr;
    p[2] = (unsigned int)(unsigned long)gc;
    p[3] = ((int)y << 16) | ((int)x & 0xFFFF);
    p[words - 1] = 0;
    unsigned char *bp = (unsigned char *)&p[4];
    bp[0] = (unsigned char)len;
    bp[1] = 0;
    for (int i = 0; i < len; i++) bp[2 + i] = (unsigned char)s[i];
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

static inline void XFreePixmap(Display *d, Pixmap p) {
    if (!d || !p) return;
    unsigned int *req = (unsigned int *)_xwire_reserve(d->fd, 8);
    req[0] = (2 << 16) | 54; /* X_FreePixmap: reqType=54, length=2 */
    req[1] = (unsigned int)p;
}

static inline void XDestroyWindow(Display *d, Window w) {
    if (!d || !w) return;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 8);
    p[0] = (2 << 16) | 4; /* X_DestroyWindow: reqType=4, length=2 */
    p[1] = (unsigned int)w;
}

static inline void XCopyArea(Display *d, Drawable src, Drawable dst, GC gc, int sx, int sy, unsigned int w, unsigned int h, int dx, int dy) {
    if (!d || !src || !dst || !gc) return;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 28);
    p[0] = (7 << 16) | 62; /* X_CopyArea: reqType=62, length=7 */
    p[1] = (unsigned int)src;
    p[2] = (unsigned int)dst;
    p[3] = (unsigned int)(unsigned long)gc;
    p[4] = ((int)sy << 16) | ((int)sx & 0xFFFF);
    p[5] = ((int)dy << 16) | ((int)dx & 0xFFFF);
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
    int data_bytes = (fmt == 32) ? (nelem << 2) : ((fmt == 16) ? (nelem << 1) : nelem);
    int pad = (-data_bytes) & 3;
    int total_bytes = 24 + data_bytes + pad;
    int words = total_bytes >> 2;

    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, words << 2);
    p[0] = (words << 16) | ((mode & 0xFF) << 8) | 18; /* X_ChangeProperty */
    p[1] = (unsigned int)w;
    p[2] = (unsigned int)prop;
    p[3] = (unsigned int)type;
    p[4] = ((unsigned int)fmt) & 0xFF;
    p[5] = (unsigned int)nelem;

    unsigned char *dp = (unsigned char *)&p[6];
    for (int i = 0; i < data_bytes; i++) dp[i] = data[i];
    for (int i = 0; i < pad; i++) dp[data_bytes + i] = 0;
}

static inline int XGrabPointer(Display *d, Window grab_w, int owner, unsigned int mask, int sync_p, int sync_k, Window confine, unsigned long cursor, Time time) {
    (void)owner; (void)sync_p; (void)sync_k; (void)confine; (void)cursor; (void)time;
    if (!d) return 1;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 24);
    p[0] = (6 << 16) | 26; /* X_GrabPointer: reqType=26, length=6 */
    p[1] = (unsigned int)grab_w;
    p[2] = (1 << 24) | (1 << 16) | (mask & 0xFFFF);
    p[3] = (unsigned int)confine;
    p[4] = (unsigned int)cursor;
    p[5] = 0;
    _xwire_flush(d->fd);
    return 0; /* GrabSuccess == 0 */
}

static inline void XUngrabPointer(Display *d, Time time) {
    if (!d) return;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 8);
    p[0] = (2 << 16) | 27; /* X_UngrabPointer */
    p[1] = (unsigned int)time;
}

static inline int XQueryPointer(Display *d, Window w, Window *r, Window *c, int *rx, int *ry, int *wx, int *wy, unsigned int *mask) {
    if (!d) return 0;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 8);
    p[0] = (2 << 16) | 38;
    p[1] = (unsigned int)w;
    _xwire_flush(d->fd);

    unsigned char rep[32];
    if (_xwire_read(d->fd, rep, 32) != 32 || rep[0] != 1)
        return 0;

    if (r) *r = *(unsigned int *)&rep[8];
    if (c) *c = *(unsigned int *)&rep[12];
    if (rx) *rx = *(short *)&rep[16];
    if (ry) *ry = *(short *)&rep[18];
    if (wx) *wx = *(short *)&rep[20];
    if (wy) *wy = *(short *)&rep[22];
    if (mask) *mask = *(unsigned short *)&rep[24];
    return 1;
}

static inline void XSendEvent(Display *d, Window w, int prop, long mask, XEvent *ev) {
    if (!d || !ev) return;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 44);
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
            *(unsigned int *)&ep[12 + (i << 2)] = (unsigned int)ev->xclient.data.l[i];
    } else {
        unsigned char *ep = (unsigned char *)&p[3];
        const unsigned char *src = (const unsigned char *)ev;
        for (int i = 0; i < 32; i++) ep[i] = src[i];
    }
}

static inline XModifierKeymap *XGetModifierMapping(Display *d) {
    (void)d;
    static KeyCode dummy[8] = { 0, 0, 0, 0, 77 /* Mod2 = NumLock */, 0, 0, 0 };
    static XModifierKeymap m = { 1, dummy };
    return &m;
}

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


/* ========================================================================= */
/* Core Window Manager Implementation                                       */
/* ========================================================================= */

#define LENGTH(X)           (sizeof(X) / sizeof(X)[0])
#define MAX(A,B)            ((A) > (B) ? (A) : (B))
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
typedef struct {
    int screen_number;
    short x_org;
    short y_org;
    short width;
    short height;
} XineramaScreenInfo;

enum {
    FLAG_FLOATING   = 1 << 0,
    FLAG_FULLSCREEN = 1 << 1,
    FLAG_DIALOG     = 1 << 2,
};

struct Client {
    Window win;
    Node *node;
    Client *hnext;
    Client *next;
    Client *prev;
    unsigned short x, y, w, h;
    unsigned short tile_x, tile_y, tile_w, tile_h;
    short workspace;
    unsigned short flags;
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
    short curtag;
    short next_split;
    short ntags;
    short tags[WORKSPACE_COUNT];
    unsigned dirty : 1;
};


static void die(const char *) __attribute__((noreturn));
static void *ecalloc(size_t, size_t);
static void arrange(Monitor *);
static void drawbar(Monitor *);
static void focus(Monitor *, Node *);
static void unmanage(Client *);
static Client *find_client(Window);
static Node *node_first_leaf(Node *);

/* Handlers bound in keys[] and buttons[] */
static void spawn(const Arg *);
static void quit(const Arg *);
static void killclient(const Arg *);
static void togglefullscreen(const Arg *);
static void togglefloating(const Arg *);
static void rotatecmd(const Arg *);
static void focusworkspace(const Arg *);
static void focusworkspace_next(const Arg *);
static void focusworkspace_prev(const Arg *);
static void focuscmd(const Arg *);
static void movecmd(const Arg *);
static void setlayoutcmd(const Arg *);
static void movemouse(const Arg *);
static void resizemouse(const Arg *);

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
static int tagmap[WORKSPACE_COUNT] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1
};
static const unsigned int MODKEY = Mod1Mask;

/* Autostart commands (run once at startup, before the event loop) */
static const char *autostart[][4] = {
    { "/bin/sh", "-c", "[ -x $HOME/.config/i3/xrandr.sh ] && $HOME/.config/i3/xrandr.sh || (command -v xrandr >/dev/null 2>&1 && xrandr --output eDP-1 --mode 1366x768 --rate 60 --pos 0x0 --output HDMI-1 --mode 1920x1080 --rate 60 --pos 1366x0 --primary)", NULL },
    { "/bin/sh", "-c", "[ -x $HOME/.config/i3/audio-autostart.sh ] && $HOME/.config/i3/audio-autostart.sh", NULL },
    { "/bin/sh", "-c", "command -v setxkbmap >/dev/null 2>&1 && setxkbmap -option caps:escape", NULL },
    { "/bin/sh", "-c", "command -v xset >/dev/null 2>&1 && xset r rate 180 250", NULL },
};

static const Key keys[] = {
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
static KeyCode key_keycodes[LENGTH(keys)];

static Display *dpy;
static Window root;
static Monitor *mons;
static Monitor *selmon;
static Workspace workspaces[WORKSPACE_COUNT];
static Client *clients;
static Atom wmatom[WM_LAST];
static Atom netatom[NET_LAST];
static int running = 1;
static unsigned int numlockmask;
static unsigned long border_active, border_inactive, bar_bg, bar_fg, bar_hl;
static GC bar_gc;

enum { NODEPOOL = 64 };
static Node  nodepool[NODEPOOL] __attribute__((aligned(64)));
static int   nodeidx;
static Node *nodefreelist;
static Client *client_freelist;

enum { CLIENT_HASH_SIZE = 64 };
static Client *client_hash[CLIENT_HASH_SIZE] __attribute__((aligned(64)));

static inline unsigned int client_hash_idx(Window w)
{
    return (unsigned int)(w ^ (w >> 6)) & (CLIENT_HASH_SIZE - 1);
}

static inline void hash_insert(Client *c)
{
    unsigned int h = client_hash_idx(c->win);
    c->hnext = client_hash[h];
    client_hash[h] = c;
}

static inline void hash_remove(Client *c)
{
    unsigned int h = client_hash_idx(c->win);
    Client **pp = &client_hash[h];
    while (*pp) {
        if (*pp == c) {
            *pp = c->hnext;
            break;
        }
        pp = &(*pp)->hnext;
    }
    c->hnext = NULL;
}


static size_t strlen(const char *s)
{
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

#define free(p) ((void)0)

static void *memset(void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

#define HEAP_SIZE (1UL * 1024)          /* 1 KB — bump allocator */
static char heap[HEAP_SIZE];
static unsigned long heap_ptr;

static void *bump_calloc(size_t nmemb, size_t size)
{
    size_t total = (nmemb * size + 7UL) & ~7UL;
    if (heap_ptr + total > HEAP_SIZE)
        return NULL;
    void *ptr = heap + heap_ptr;
    heap_ptr += total;
    return ptr;
}
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
    long pid = sys_fork();
    if (pid == 0) {
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
    if (pid > 0)
        sys_wait4(pid, NULL, 0, NULL);
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
    n->parent  = NULL;
    n->u.ch.first   = NULL;
    n->u.ch.second  = NULL;
    return n;
}

static void node_free(Node *n)
{
    if (!n) return;
    if (n->is_leaf && n->u.client && n->u.client->node == n)
        n->u.client->node = NULL;
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
    if (n->u.client && n->u.client->node == n)
        n->u.client->node = NULL;

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
    c->node = newleaf;

    if (!focus || !ws->root) {
        ws->root = newleaf;
        return newleaf;
    }

    newnode = node_new();
    newnode->is_leaf = 0;

    Node *fparent = focus->parent;
    if (m->next_split >= 0) {
        newnode->split = m->next_split;
        m->next_split  = -1;
    } else {
        newnode->split = fparent ? !fparent->split : SPLIT_VERTICAL;
    }
    newnode->parent = fparent;
    newnode->u.ch.first  = focus;
    newnode->u.ch.second = newleaf;
    focus->parent   = newnode;
    newleaf->parent = newnode;

    if (fparent) {
        if (fparent->u.ch.first == focus)
            fparent->u.ch.first = newnode;
        else
            fparent->u.ch.second = newnode;
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
        Node *parent = p->parent;
        if (parent->split == orient) {
            int on_first = (parent->u.ch.first == p);
            if (on_first != want_first) {
                c = want_first ? parent->u.ch.first : parent->u.ch.second;

                int sub_pos = -1;
                Node *q = n;
                while (q != parent) {
                    Node *qp = q->parent;
                    if (qp && qp->split != orient)
                        sub_pos = (qp->u.ch.first == q) ? 0 : 1;
                    q = qp;
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
        p = parent;
    }
    return NULL;
}

Node *node_first_leaf(Node *n)
{
    if (!n) return NULL;
    while (!n->is_leaf) n = n->u.ch.first;
    return n;
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

    if (!m) return;

    for (c = clients; c; c = c->next) {
        if (!(c->flags & FLAG_FLOATING)) continue;
        if (c->workspace != m->curtag) continue;

        XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
        XSetWindowBorderWidth(dpy, c->win, borderpx);
        XSetWindowBorder(dpy, c->win, (c->node && c->node != m->ws->focus) ? border_inactive : border_active);
    }
}
void arrange_node(Node *n, int x, int y, int w, int h)
{
    Client *c;
    if (!n) return;

    if (n->is_leaf) {
        c = n->u.client;
        if (!c || (c->flags & (FLAG_FULLSCREEN | FLAG_FLOATING)))
            return;
        c->x = x + borderpx;
        c->y = y + borderpx;
        c->w = MAX(1, w - (borderpx << 1));
        c->h = MAX(1, h - (borderpx << 1));
        XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
        return;
    }

    int half_gap = gappx > 0 ? (gappx >> 1) : 0;
    if (n->split == SPLIT_VERTICAL) {
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

    if (m->barwin && !(m->ws->focus && m->ws->focus->u.client &&
        (m->ws->focus->u.client->flags & FLAG_FULLSCREEN))) {
        int bw = m->w - (gappx << 1);
        if (m->bar_width != bw) {
            m->bar_width = bw;
            XMoveResizeWindow(dpy, m->barwin,
                              m->x + gappx, m->y + gappx,
                              m->bar_width, BAR_HEIGHT);
        }
    }

    wx = m->x + gappx;
    wy = m->y + gappx + BAR_HEIGHT + gappx;
    ww = m->w - gappx - borderpx;
    wh = m->h - BAR_HEIGHT - (gappx << 1) - borderpx;

    if (m->ws->root)
        arrange_node(m->ws->root, wx, wy, ww, wh);

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
    c->node = NULL;
    c->hnext = NULL;
    c->x   = wa->x;
    c->y   = wa->y;
    c->w   = wa->width;
    c->h   = wa->height;
    c->workspace = selmon ? selmon->curtag : 0;
    hash_insert(c);

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
                     StructureNotifyMask;
    swa.background_pixel = bar_bg;
    XSetWindowBorderWidth(dpy, w, borderpx);
    XSetWindowBorder(dpy, w, border_inactive);
    XChangeWindowAttributes(dpy, w, CWEventMask | CWBackPixel, &swa);

    unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask | LockMask };
    for (int i = 0; i < 4; i++) {
        XGrabButton(dpy, Button1, MODKEY | modifiers[i], w, False,
                    ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                    GrabModeAsync, GrabModeAsync, None, None);
        XGrabButton(dpy, Button3, MODKEY | modifiers[i], w, False,
                    ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                    GrabModeAsync, GrabModeAsync, None, None);
    }

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
    Node *newfocus;

    if (!c) return;

    hash_remove(c);

    for (m = mons; m; m = m->next) {
        if (m->curtag == c->workspace) break;
    }
    if (m) {
        if (m->ws->fs_client == c)
            m->ws->fs_client = NULL;
        if (c->node) {
            newfocus = node_detach(c->node, m->ws);
            m->ws->focus = newfocus;
            node_free(c->node);
            m->dirty = 1;
            arrange(m);
        } else if (c->flags & FLAG_FLOATING) {
            m->dirty = 1;
            arrange(m);
        }
    }

    if (c->next) c->next->prev = c->prev;
    if (c->prev) c->prev->next = c->next;
    else         clients = c->next;

    XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
    c->next = client_freelist;
    c->prev = NULL;
    c->node = NULL;
    client_freelist = c;
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
    XSetInputFocus(dpy, w, RevertToPointerRoot, CurrentTime);
    XRaiseWindow(dpy, w);
    XChangeProperty(dpy, root, netatom[NET_ACTIVE_WINDOW],
                    XA_WINDOW, 32, PropModeReplace,
                    (unsigned char *)&w, 1);
}

Client *find_client(Window w)
{
    if (__builtin_expect(!w || w == root, 0)) return NULL;
    unsigned int h = client_hash_idx(w);
    for (Client *c = client_hash[h]; c; c = c->hnext)
        if (c->win == w) return c;
    return NULL;
}

void movemouse(const Arg *arg)
{
    Client *c = NULL;
    int di, ox = 0, oy = 0;
    unsigned int du;
    Window dw;
    XEvent ev;

    if (arg && arg->v) {
        const XEvent *bev = (const XEvent *)arg->v;
        c = find_client(bev->xbutton.window);
    }
    if (!c) {
        SELCLIENT(c);
    }

    if (!(c->flags & FLAG_FLOATING)) {
        Arg a = {0};
        togglefloating(&a);
    }

    if (arg && arg->v) {
        const XEvent *bev = (const XEvent *)arg->v;
        ox = bev->xbutton.x_root - c->x;
        oy = bev->xbutton.y_root - c->y;
    } else {
        if (!XQueryPointer(dpy, root, &dw, &dw, &ox, &oy, &di, &di, &du))
            return;
        ox -= c->x;
        oy -= c->y;
    }

    if (XGrabPointer(dpy, root, False,
                     PointerMotionMask | ButtonReleaseMask,
                     GrabModeAsync, GrabModeAsync, None, None,
                     CurrentTime) != GrabSuccess)
        return;

    for (;;) {
        XNextEvent(dpy, &ev);
        switch (ev.type) {
        case MotionNotify:
            XMoveWindow(dpy, c->win,
                        ev.xmotion.x_root - ox,
                        ev.xmotion.y_root - oy);
            if (selmon && selmon->barwin)
                XRaiseWindow(dpy, selmon->barwin);
            _xwire_flush(dpy->fd);
            break;
        case ButtonRelease:
            c->x = ev.xbutton.x_root - ox;
            c->y = ev.xbutton.y_root - oy;
            {
                int midx = c->x + (c->w >> 1);
                int midy = c->y + (c->h >> 1);
                Monitor *target_mon = selmon;
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
                    if (c->x + c->w + (borderpx << 1) > target_mon->x + target_mon->w)
                        c->x = target_mon->x + target_mon->w - c->w - (borderpx << 1);
                    if (c->y + c->h + (borderpx << 1) > target_mon->y + target_mon->h)
                        c->y = target_mon->y + target_mon->h - c->h - (borderpx << 1);
                    if (c->x < target_mon->x) c->x = target_mon->x;
                    if (c->y < target_mon->y) c->y = target_mon->y;
                    if (old_m != target_mon)
                        arrange(old_m);
                }
            }
            XMoveWindow(dpy, c->win, c->x, c->y);
            XUngrabPointer(dpy, CurrentTime);
            focus(selmon, selmon->ws->focus);
            arrange(selmon);
            return;
        }
    }
}

void resizemouse(const Arg *arg)
{
    Client *c = NULL;
    XEvent ev;

    if (arg && arg->v) {
        const XEvent *bev = (const XEvent *)arg->v;
        c = find_client(bev->xbutton.window);
    }
    if (!c) {
        SELCLIENT(c);
    }

    if (!(c->flags & FLAG_FLOATING)) {
        Arg a = {0};
        togglefloating(&a);
    }

    if (XGrabPointer(dpy, root, False,
                     PointerMotionMask | ButtonReleaseMask,
                     GrabModeAsync, GrabModeAsync, None, None,
                     CurrentTime) != GrabSuccess)
        return;

    for (;;) {
        XNextEvent(dpy, &ev);
        switch (ev.type) {
        case MotionNotify:
            XResizeWindow(dpy, c->win,
                          MAX(1, ev.xmotion.x_root - c->x),
                          MAX(1, ev.xmotion.y_root - c->y));
            if (selmon && selmon->barwin)
                XRaiseWindow(dpy, selmon->barwin);
            _xwire_flush(dpy->fd);
            break;
        case ButtonRelease:
            c->w = MAX(1, ev.xbutton.x_root - c->x);
            c->h = MAX(1, ev.xbutton.y_root - c->y);
            {
                Monitor *m = selmon;
                for (Monitor *tm = mons; tm; tm = tm->next) {
                    if (c->x >= tm->x && c->x < tm->x + tm->w &&
                        c->y >= tm->y && c->y < tm->y + tm->h) {
                        m = tm;
                        break;
                    }
                }
                if (c->x + c->w + (borderpx << 1) > m->x + m->w)
                    c->w = m->x + m->w - c->x - (borderpx << 1);
                if (c->y + c->h + (borderpx << 1) > m->y + m->h)
                    c->h = m->y + m->h - c->y - (borderpx << 1);
                c->w = MAX(1, c->w);
                c->h = MAX(1, c->h);
            }
            XResizeWindow(dpy, c->win, c->w, c->h);
            XUngrabPointer(dpy, CurrentTime);
            focus(selmon, selmon->ws->focus);
            arrange(selmon);
            return;
        }
    }
}
static int _xinerama_opcode = 0;

static inline int _xwire_query_xinerama(Display *d) {
    if (!d) return 0;
    if (_xinerama_opcode > 0) return _xinerama_opcode;
    if (_xinerama_opcode < 0) return 0;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 16);
    p[0] = (4 << 16) | 98;
    p[1] = 8;
    const char *xname = "XINERAMA";
    for (int i = 0; i < 8; i++) ((char *)&p[2])[i] = xname[i];
    _xwire_flush(d->fd);

    unsigned char rep[32];
    if (_xwire_read(d->fd, rep, 32) != 32 || rep[0] != 1) {
        _xinerama_opcode = -1;
        return 0;
    }
    if (!rep[8]) {
        _xinerama_opcode = -1;
        return 0;
    }
    _xinerama_opcode = (int)rep[9];
    return _xinerama_opcode;
}

static inline int XineramaIsActive(Display *d) {
    int op = _xwire_query_xinerama(d);
    if (!op) return 0;
    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 4);
    *p = (1 << 16) | (4 << 8) | op;
    _xwire_flush(d->fd);

    unsigned char rep[32];
    if (_xwire_read(d->fd, rep, 32) != 32 || rep[0] != 1)
        return 0;
    unsigned int state = *(unsigned int *)&rep[8];
    return (state != 0);
}

static inline XineramaScreenInfo *XineramaQueryScreens(Display *d, int *number) {
    *number = 0;
    int op = _xwire_query_xinerama(d);
    if (!op) return (void *)0;

    unsigned int *p = (unsigned int *)_xwire_reserve(d->fd, 4);
    *p = (1 << 16) | (5 << 8) | op;
    _xwire_flush(d->fd);

    unsigned char rep[32];
    if (_xwire_read(d->fd, rep, 32) != 32 || rep[0] != 1)
        return (void *)0;

    unsigned int count = *(unsigned int *)&rep[8];
    if (count == 0 || count > 16) return (void *)0;

    static XineramaScreenInfo screens[16];
    for (unsigned int i = 0; i < count; i++) {
        unsigned char s[8];
        if (_xwire_read(d->fd, s, 8) != 8) return (void *)0;
        screens[i].screen_number = (int)i;
        screens[i].x_org  = *(short *)&s[0];
        screens[i].y_org  = *(short *)&s[2];
        screens[i].width  = *(unsigned short *)&s[4];
        screens[i].height = *(unsigned short *)&s[6];
    }
    *number = (int)count;
    return screens;
}

static int isuniquegeom(XineramaScreenInfo *unique, int n, XineramaScreenInfo *info)
{
    for (int i = 0; i < n; i++)
        if (unique[i].x_org  == info->x_org  &&
            unique[i].y_org  == info->y_org  &&
            unique[i].width  == info->width  &&
            unique[i].height == info->height)
            return 0;
    return 1;
}

static Monitor *
monitor_new(int num, int x, int y, int w, int h)
{
    Monitor *m = ecalloc(1, sizeof(Monitor));
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
    Monitor *m;
    mons = NULL;

    int n = 0;
    XineramaScreenInfo *info = NULL;
    if (XineramaIsActive(dpy)) {
        info = XineramaQueryScreens(dpy, &n);
    }

    if (info && n > 0) {
        static XineramaScreenInfo unique[16];
        int nunique = 0;

        for (int i = 0; i < n && nunique < 16; i++)
            if (isuniquegeom(unique, nunique, &info[i]))
                unique[nunique++] = info[i];

        if (nunique <= 1)
            tagmap[WORKSPACE_COUNT - 1] = 0;

        for (int i = 0; i < nunique; i++) {
            m = monitor_new(i, unique[i].x_org, unique[i].y_org,
                            unique[i].width, unique[i].height);
            m->next = mons;
            mons = m;
            if (i == 0) selmon = m;
        }
    } else {
        tagmap[WORKSPACE_COUNT - 1] = 0;
        mons = monitor_new(0, 0, 0,
                           DisplayWidth(dpy, XDefaultScreen(dpy)),
                           DisplayHeight(dpy, XDefaultScreen(dpy)));
        selmon = mons;
    }
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
    Monitor *m, *prev;
    Workspace *oldws;
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
        if (prev != m) { prev->dirty = 1; m->dirty = 1; }
        update_bar_visibility(m);
        arrange(m);
        if (prev != m) {
            update_bar_visibility(prev);
            arrange(prev);
        }
        return;
    }

    oldws = m->ws;
    int oldtag = m->curtag;

    m->curtag = tag;
    m->ws     = &workspaces[tag];
    m->dirty  = 1;

    if (m->ws->focus && m->ws->focus->u.client)
        focus(m, m->ws->focus);
    else if (m->ws->root) {
        Node *first = node_first_leaf(m->ws->root);
        if (first)
            focus(m, first);
    } else {
        XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
        XChangeProperty(dpy, root, netatom[NET_ACTIVE_WINDOW],
                        XA_WINDOW, 32, PropModeReplace,
                        (unsigned char *)&(Window){0}, 1);
    }

    update_bar_visibility(m);
    arrange(m);

    if (oldws != m->ws) {
        node_hide_foreach(oldws->root);
        for (Client *fc = clients; fc; fc = fc->next) {
            if ((fc->flags & FLAG_FLOATING) && fc->workspace == oldtag)
                XMoveWindow(dpy, fc->win, -9999, -9999);
        }
    }

    if (prev != m) {
        prev->dirty = 1;
        update_bar_visibility(prev);
        arrange(prev);
    }
}

void focusworkspace_next(const Arg *arg)
{
    if (!selmon) return;
    int tag = selmon->curtag + 1;
    if (tag >= WORKSPACE_COUNT) tag = 0;
    Arg a = {.i = tag};
    focusworkspace(&a);
}

void focusworkspace_prev(const Arg *arg)
{
    if (!selmon) return;
    int tag = selmon->curtag - 1;
    if (tag < 0) tag = WORKSPACE_COUNT - 1;
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

    if (!bar_gc)
        bar_gc = XCreateGC(dpy, m->barwin, 0, NULL);

    d = m->barpm ? m->barpm : m->barwin;

    XSetForeground(dpy, bar_gc, bar_bg);
    XFillRectangle(dpy, d, bar_gc, 0, 0, m->bar_width, BAR_HEIGHT);

    unsigned long cur_fg = bar_bg;
    int half_sw = (sw >> 1) - 4;
    int ypos = (BAR_HEIGHT >> 1) + 5;
    for (idx = 0, x = 0; idx < m->ntags; idx++, x += sw) {
        i = m->tags[idx];
        char ch = (char)('1' + i);
        unsigned long fg = (m == selmon && m->curtag == i) ? bar_fg : bar_hl;
        if (fg != cur_fg) {
            XSetForeground(dpy, bar_gc, fg);
            cur_fg = fg;
        }
        XDrawString(dpy, d, bar_gc, x + half_sw, ypos, &ch, 1);
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
        m->bar_width = m->w - (gappx << 1);
        wa.override_redirect = True;
        wa.background_pixel = 0;
        wa.event_mask = ExposureMask | ButtonPressMask;

        m->barwin = XCreateWindow(dpy, root,
                                  m->x + gappx, m->y + gappx,
                                  m->bar_width, BAR_HEIGHT, 0,
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
    unsigned int kc = e->xkey.keycode;
    unsigned int state = CLEANMASK(e->xkey.state);

    for (size_t i = 0; i < LENGTH(keys); i++)
        if (key_keycodes[i] == kc && keys[i].mod == state) {
            keys[i].func(&keys[i].arg);
            return;
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

        if (e->xbutton.button == Button1) {
            Arg a = {.v = e};
            movemouse(&a);
        } else if (e->xbutton.button == Button3) {
            Arg a = {.v = e};
            resizemouse(&a);
        }
        return;
    }

    if (c && c->node) {
        for (m = mons; m; m = m->next) {
            if (m->curtag == c->workspace) {
                focus(m, c->node);
                arrange(m);
                return;
            }
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
    for (m = mons; m; m = m->next) {
        if (m->curtag == c->workspace) break;
    }
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

    if (c) {
        if (c->flags & FLAG_FLOATING) {
            c->x = ev->x; c->y = ev->y;
            c->w = ev->width; c->h = ev->height;
            XConfigureWindowFromReq(dpy, ev);
        }
    } else {
        XConfigureWindowFromReq(dpy, ev);
    }
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
        fw = ((unsigned int)selmon->w * 39322U) >> 16;
        fh = ((unsigned int)selmon->h * 39322U) >> 16;
        c->x = selmon->x + ((selmon->w - fw) >> 1);
        c->y = selmon->y + ((selmon->h - fh) >> 1);
        if (c->x + fw + (borderpx << 1) > selmon->x + selmon->w)
            c->x = selmon->x + selmon->w - fw - (borderpx << 1);
        if (c->y + fh + (borderpx << 1) > selmon->y + selmon->h)
            c->y = selmon->y + selmon->h - fh - (borderpx << 1);
        if (c->x < selmon->x) c->x = selmon->x;
        if (c->y < selmon->y) c->y = selmon->y;
        c->w = fw; c->h = fh;
        c->workspace = selmon->curtag;
        XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
        XSetWindowBorderWidth(dpy, c->win, borderpx);
        XSetWindowBorder(dpy, c->win, border_active);
        XRaiseWindow(dpy, c->win);
    } else {
        c->x = c->tile_x; c->y = c->tile_y;
        c->w = c->tile_w; c->h = c->tile_h;
        if (!c->node)
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
    c->node = nl;

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
        Monitor *target_m = NULL;
        for (Monitor *tm = mons; tm; tm = tm->next) {
            if (tm->curtag == tag) {
                target_m = tm;
                break;
            }
        }
        if (target_m) {
            arrange(target_m);
        } else {
            XMoveWindow(dpy, c->win, -9999, -9999);
        }
        if (!newfocus) {
            XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
            XChangeProperty(dpy, root, netatom[NET_ACTIVE_WINDOW],
                            XA_WINDOW, 32, PropModeReplace,
                            (unsigned char *)&(Window){0}, 1);
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
        n->u.client->node = n;
        target->u.client->node = target;
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
    }

    modifiers[2] = numlockmask;
    modifiers[3] = numlockmask | LockMask;

    XUngrabKey(dpy, AnyKey, AnyModifier, root);

    for (i = 0; i < LENGTH(keys); i++) {
        keysym = XKeysymToKeycode(dpy, keys[i].keysym);
        key_keycodes[i] = (KeyCode)keysym;
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
    XSetWindowAttributes wa;
    Atom supported[] = {
        netatom[NET_WM_STATE],
        netatom[NET_WM_STATE_FULLSCREEN],
        netatom[NET_ACTIVE_WINDOW],
    };

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

    dpy = XOpenDisplay(NULL);
    if (!dpy) die("fiwm: cannot open display\n");
    root = RootWindow(dpy, XDefaultScreen(dpy));
    border_active   = parse_color(colors[COL_BORDER_ACTIVE]);
    border_inactive = parse_color(colors[COL_BORDER_INACTIVE]);
    bar_bg          = parse_color(colors[COL_BAR_BG]);
    bar_fg          = parse_color(colors[COL_BAR_FG]);
    bar_hl          = parse_color(colors[COL_BAR_HL]);


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

    wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask |
                    EnterWindowMask | ButtonPressMask |
                    StructureNotifyMask | PropertyChangeMask;
    XChangeWindowAttributes(dpy, root, CWEventMask, &wa);

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
        hash_remove(c);
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

    XSync(dpy, False);
    XCloseDisplay(dpy);

    if (xorg_pid > 0) {
        sys_kill(xorg_pid, 15);
        long status;
        syscall4(SYS_WAIT4, xorg_pid, (long)&status, 0, 0);
        xorg_pid = 0;
    }
}

static char *custom_envp[160];
static char display_env[] = "DISPLAY=:0";

static void setup_environ(char **envp)
{
    int has_display = 0;
    int count = 0;

    if (envp) {
        for (char **ep = envp; *ep && count < 150; ep++) {
            if (*(const unsigned long *)*ep == 0x3d59414c50534944ULL) {
                has_display = 1;
                env_display = *ep + 8;
            }
            custom_envp[count++] = *ep;
        }
    }

    if (!has_display) {
        custom_envp[count++] = display_env;
    }
    custom_envp[count] = NULL;
    envp_global = custom_envp;
}

__attribute__((used, noinline))
int wm_main(int argc, char **argv, char **envp)
{

    setup_environ(envp);

    if (argc == 2 && argv[1][0] == '-' && argv[1][1] == 'v' && !argv[1][2]) {
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
