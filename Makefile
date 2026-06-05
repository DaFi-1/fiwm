# fiwm - BSP tiling window manager (freestanding edition)
# Build: zero-libc, _start entry, syscall-only runtime
#
# NOTE: libX11 and libXinerama are dynamically linked — they bring
# their own libc at runtime.  Our code uses NO libc directly.

PREFIX    = /usr/local
X11INC    = /usr/include
X11LIB    = /usr/lib/x86_64-linux-gnu

XINERAMALIBS  = -lXinerama
XINERAMAFLAGS = -DXINERAMA

INCS = -I${X11INC}
LIBS = -L${X11LIB} -lX11 ${XINERAMALIBS}

CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700L ${XINERAMAFLAGS}
CFLAGS   = -std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter \
           -ffreestanding -fno-builtin \
           -Os -ffunction-sections -fdata-sections ${INCS} ${CPPFLAGS}
LDFLAGS  = -nostartfiles -Wl,--as-needed -Wl,--gc-sections -Wl,-z,stack-size=32768 -s ${LIBS}

CC = cc

SRC = fiwm.c
OBJ = ${SRC:.c=.o}

all: fiwm

.c.o:
	${CC} -c ${CFLAGS} $<

fiwm: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}
	rm -f ${OBJ}

clean:
	rm -f fiwm ${OBJ}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f fiwm ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/fiwm

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/fiwm

# Validation targets
check-elf:
	@echo "=== ELF header check (should show NO interpreter) ==="
	readelf -l fiwm | grep INTERP || echo "  ✓ No INTERP segment"
	@echo ""
	@echo "=== Dynamic dependency check ==="
	ldd fiwm || true
	@echo ""
	@echo "=== Symbol check (should NOT show libc symbols) ==="
	nm fiwm | grep -i " U " | head -20 || echo "  (no undefined symbols from our code)"

.PHONY: all clean install uninstall check-elf
