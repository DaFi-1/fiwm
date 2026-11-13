#!/usr/bin/env python3
import ctypes, sys, time, signal

x11 = ctypes.CDLL('libX11.so.6')
x11.XOpenDisplay.restype = ctypes.c_void_p
x11.XOpenDisplay.argtypes = [ctypes.c_void_p]
x11.XDefaultRootWindow.restype = ctypes.c_ulong
x11.XDefaultRootWindow.argtypes = [ctypes.c_void_p]
x11.XCreateSimpleWindow.restype = ctypes.c_ulong
x11.XCreateSimpleWindow.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_int, ctypes.c_int, ctypes.c_uint, ctypes.c_uint, ctypes.c_uint, ctypes.c_ulong, ctypes.c_ulong]
x11.XFlush.argtypes = [ctypes.c_void_p]

disp = x11.XOpenDisplay(None)
if not disp:
    sys.exit(1)
root = x11.XDefaultRootWindow(disp)
w = int(sys.argv[1]) if len(sys.argv) > 1 else 1600
h = int(sys.argv[2]) if len(sys.argv) > 2 else 600
win = x11.XCreateSimpleWindow(disp, root, -9999, -9999, w, h, 0, 0, 0)
x11.XFlush(disp)

print(win, flush=True)

signal.signal(signal.SIGTERM, lambda s, f: sys.exit(0))
while True:
    time.sleep(1)
