# fiwm - Fast Independent Window Manager
# Zero-libc, freestanding x86_64 Linux binary
# Pure X11 wire protocol over UNIX domain socket

PREFIX  = /usr/local
CC      = cc
MARCH  ?= native

CFLAGS  = -std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter \
          -ffreestanding -fno-builtin -fno-stack-protector -fomit-frame-pointer \
          -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-ident \
          -fmerge-all-constants -fno-plt -flto -fwhole-program -march=${MARCH} \
          -Os -ffunction-sections -fdata-sections

LDFLAGS = -nostdlib -static -flto -Wl,--gc-sections -Wl,--build-id=none \
          -Wl,--no-eh-frame-hdr -Wl,-z,norelro -Wl,-n \
          -Wl,-z,stack-size=4096 -s

all: fiwm

fiwm: fiwm.c
	${CC} ${CFLAGS} -o $@ $< ${LDFLAGS}

# Backward compatibility alias
fiwm-wire: fiwm
	ln -sf fiwm fiwm-wire

clean:
	rm -f fiwm fiwm-wire *.o

install: fiwm
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f fiwm ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/fiwm

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/fiwm

check-elf: fiwm
	@echo "=== ELF header check (should show NO interpreter) ==="
	readelf -l fiwm | grep INTERP || echo "  ✓ No INTERP segment (pure static freestanding)"
	@echo ""
	@echo "=== Binary size ==="
	ls -lh fiwm
	@echo ""
	@echo "=== Section headers ==="
	size fiwm

test: fiwm
	./test_wire.sh

test-shortcuts: fiwm
	./test_all_shortcuts.sh

.PHONY: all clean install uninstall check-elf test test-shortcuts
