/*
 * fiwm_poc.s - Pure x86_64 Assembly Dual-Monitor & 9-Workspace Window Manager
 *
 * Capabilities:
 * - 9 Workspaces: Switch with Alt+1..9, Move window with Alt+Shift+1..9
 * - Dual Monitor Autodetection via X11 Xinerama wire protocol extension:
 *   * If 2 monitors detected: Monitor 0 hosts WS 1-8, Monitor 1 hosts WS 9
 *   * If 1 monitor: Hosts all WS 1-9
 * - Independent BSP Trees: Each of the 9 workspaces maintains its own
 *   independent hierarchical BSP tree!
 * - Hiding / Showing: Switching workspaces cleanly unmaps old windows
 *   and maps / arranges windows belonging to the newly active workspace.
 * - Targeted Splits: Alt+b (Vertical) and Alt+v (Horizontal)
 * - Shortcuts:
 *   * Alt+1..9: Focus workspace 1 to 9
 *   * Alt+Shift+1..9: Move focused window to workspace 1 to 9
 *   * Alt+w / Alt+Enter: Spawn terminal (st)
 *   * Alt+d: Spawn menu (nviciod.sh / dmenu)
 *   * Alt+q: Kill active window
 *   * Alt+Shift+q: Cleanly exit WM
 *   * Alt+f: Fullscreen active window
 *   * Alt+j / Alt+k: Focus cycle
 *
 * Zero C, zero libc, pure x86_64 assembly (~4 KB binary!).
 */

.global _start
.intel_syntax noprefix

.equ SYS_READ,    0
.equ SYS_WRITE,   1
.equ SYS_CLOSE,   3
.equ SYS_POLL,    7
.equ SYS_FORK,    57
.equ SYS_VFORK,   58
.equ SYS_EXECVE,  59
.equ SYS_EXIT,    60
.equ SYS_WAIT4,   61
.equ SYS_SOCKET,  41
.equ SYS_CONNECT, 42

.equ AF_UNIX,     1
.equ SOCK_STREAM, 1

.equ WORKSPACE_COUNT, 9
.equ MAX_MONITORS,    2
.equ GAP_PX,          4
.equ BAR_HEIGHT,      18
.equ BORDER_ACTIVE,   0x005577
.equ BORDER_INACTIVE, 0x2D2D2D
.equ BG_COLOR,        0x000000

/*
 * Compact Node Structure (32 bytes aligned - 50% smaller!):
 * offset 0:  dword client_win (if leaf) OR 0 (if internal split node) [4 bytes]
 * offset 4:  byte  packed_info: (ws_idx << 1) | split_type            [1 byte]
 *                  - bit 0:    split_type (0 = vertical, 1 = horizontal)
 *                  - bits 1-4: ws_idx (0..8)
 * offset 5:  bytes pad                                                [3 bytes]
 * offset 8:  qword first_child (pointer)                              [8 bytes]
 * offset 16: qword second_child (pointer)                             [8 bytes]
 * offset 24: qword parent (pointer)                                   [8 bytes]
 *
 * Bit-Packed Hardware State Register ebp / bpl (8-bit Bitfield — Zero RAM!):
 * - bit 0:    is_fullscreen  (0 = tiled, 1 = fullscreen)
 * - bits 1-2: next_split_sel (0 = auto, 1 = Vertical, 2 = Horizontal)
 * - bit 3:    is_dual_mon    (0 = 1 monitor, 1 = 2 monitors)
 * - bit 4:    disp_found     (0 = no DISPLAY in env, 1 = found)
 * - bit 5:    is_floating    (0 = tiled, 1 = floating)
 */
.equ NODE_WIN,     0
.equ NODE_INFO,    4
.equ NODE_FIRST,   8
.equ NODE_SECOND,  16
.equ NODE_PARENT,  24
.equ NODE_SIZE,    32

.text
_start:
    xor r14d, r14d              /* r14 = permanent zero-RAM cur_ws (0..8) */
    xor ebp, ebp                /* bpl = permanent 8-bit bitwise state register (Zero RAM!) */
    mov r8, [rsp]               /* argc */
    lea r9, [rsp + 8*r8 + 16]   /* r9 = envp */
    mov [saved_envp], r9

find_display:
    mov r10, [r9]
    test r10, r10
    jz display_done

    mov rax, [r10]
    mov r11, 0x3d59414c50534944  /* "DISPLAY=" */
    cmp rax, r11
    jne next_env

    lea r10, [r10 + 8]
    cmp byte ptr [r10], ':'
    jne next_env
    or bpl, 16                  /* set bit 4 (disp_found = 1) */
    inc r10

    lea rdi, [sock_path_num]
copy_disp:
    mov al, [r10]
    test al, al
    jz display_done
    cmp al, '.'
    je display_done
    mov [rdi], al
    inc rdi
    inc r10
    jmp copy_disp

next_env:
    add r9, 8
    jmp find_display

display_done:
    /* No DISPLAY in the environment (TTY login): give children DISPLAY=:0,
       otherwise st & co. cannot connect to the X server we talk to. */
    test bpl, 16                /* check bit 4 (disp_found) */
    jnz env_ok
    mov rsi, [saved_envp]
    lea rdi, [env_buf]
    mov ecx, 18
env_copy:
    mov rax, [rsi]
    test rax, rax
    jz env_copied
    mov [rdi], rax
    add rsi, 8
    add rdi, 8
    dec ecx
    jnz env_copy
env_copied:
    lea rax, [disp_env_str]
    mov [rdi], rax
    mov qword ptr [rdi + 8], 0
    lea rax, [env_buf]
    mov [saved_envp], rax
env_ok:
    xor r12d, r12d              /* r12d = connect attempts (0 = Xorg not spawned) */

do_connect:
    /* 1. sys_socket(AF_UNIX, SOCK_STREAM, 0) - 8-bit imm8 loads! */
    push SYS_SOCKET
    pop rax
    push AF_UNIX
    pop rdi
    push SOCK_STREAM
    pop rsi
    xor edx, edx
    syscall
    test eax, eax
    js exit_fail
    mov r13d, eax               /* r13d = permanent zero-RAM socket fd! */

    /* 2. sys_connect(sock_fd, &sock_addr, sizeof(sock_addr)) */
    mov al, SYS_CONNECT         /* upper bits of eax are 0 after socket fd < 256! */
    mov edi, r13d
    lea rsi, [sock_addr]
    mov dl, 110                 /* edx was 0 from xor edx, edx! */
    syscall
    test eax, eax
    jns connected_ok

    /* Connect failed: close the socket (rdi already == r13d!) */
    push SYS_CLOSE
    pop rax
    syscall

    /* Autostart: only for display ":0" and only once */
    test r12d, r12d
    jnz retry_wait
    cmp byte ptr [sock_path_num], '0'
    jne exit_fail
    cmp byte ptr [sock_path_num + 1], 0
    jne exit_fail
    call start_xorg
    test rax, rax
    jle exit_fail
    mov [xorg_pid], rax
    mov r12d, 1

retry_wait:
    cmp r12d, 151               /* up to 150 retries x 20ms = 3s */
    jg exit_fail
    inc r12d
    push SYS_POLL               /* poll(NULL, 0, 20): sleep 20ms */
    pop rax
    xor edi, edi
    xor esi, esi
    push 20
    pop rdx
    syscall
    jmp do_connect

connected_ok:
    /* 3. Send X11 Setup Request (12 bytes) - eax == 0 and rdi == r13d from connect! */
    mov al, SYS_WRITE
    lea rsi, [x11_setup_req]
    mov dl, 12                  /* edx was 110 (< 256), so upper 24 bits are 0! */
    syscall

    /* 4. Read X11 Setup Reply Header (8 bytes) - rdi still == r13d! */
    xor eax, eax                /* SYS_READ = 0 (2 bytes!) */
    lea rsi, [in_buf]
    mov dl, 8                   /* edx was 12 (< 256)! */
    syscall
    cmp byte ptr [in_buf], 1
    jne exit_fail

    movzx ecx, word ptr [in_buf + 6]
    shl ecx, 2                  /* ecx = remaining setup reply bytes to drain */

    /* Read first chunk into in_buf (up to 576 bytes - fits in 1 KB I/O overlay!) */
    xor eax, eax                /* SYS_READ = 0, rdi still == r13d! */
    lea rsi, [in_buf]
    mov edx, 576
    cmp edx, ecx
    cmovg edx, ecx
    push rcx
    push rdx
    syscall
    pop rdx
    pop rcx
    sub ecx, eax                /* remaining bytes */

    /* Parse Screen Info from first chunk */
    movzx eax, word ptr [in_buf + 16]   /* vendor_len */
    add eax, 3
    and eax, -4
    add eax, 32
    movzx edx, byte ptr [in_buf + 21]   /* num_formats */
    shl edx, 3
    add eax, edx                        /* screen_offset */

    mov edx, [in_buf + rax]
    mov [root_win], edx

    movzx edx, word ptr [in_buf + rax + 20]
    mov [screen_w], edx
    movzx edx, word ptr [in_buf + rax + 22]
    mov [screen_h], edx

    /* Extract resource_id_base (bar_win[0..1], bar_gc (bg), bar_gc+1 (act), bar_gc+2 (inact)) */
    mov edx, [in_buf + 4]
    mov [bar_win + 0], edx
    inc edx
    mov [bar_win + 4], edx
    inc edx
    mov [bar_gc], edx

    /* Drain all remaining setup reply bytes from socket (rdi still == r13d!) */
drain_setup_loop:
    test ecx, ecx
    jle setup_drained
    mov edx, 576
    cmp edx, ecx
    cmovg edx, ecx
    push rcx
    xor eax, eax                /* SYS_READ = 0 */
    lea rsi, [in_buf]
    syscall
    pop rcx
    test eax, eax
    jle setup_drained
    sub ecx, eax
    jmp drain_setup_loop

setup_drained:
    /* 6. Autostart (i3 parity): keyboard layout + repeat rate.
       Run BEFORE grabbing keys on root so setxkbmap's MappingNotify
       does not ungrab or race with our key grabs! */
    lea rsi, [sh_argv_autostart]
    call spawn_cmd

    /* Detect Dual Monitor (Xinerama) */
    call detect_monitors

    /* Create and map top status bar + SelectInput on root + Grab all 72 keys in ONE batched syscall! */
    call init_bar
    lea r15, [out_buf]

event_loop:
    /* Check if event_queue still has buffered events */
    mov eax, [ev_q_head]
    cmp eax, [ev_q_tail]
    jl ev_pop_buffered

    /* Coalesced I/O Flush: send all pending X11 commands before blocking on sys_read */
    call flush_out_buf

    /* Buffer is empty, read next chunk of events (up to 64 events = 2048 bytes) */
    xor eax, eax                /* SYS_READ = 0 (2 bytes!) */
    mov edi, r13d
    lea rsi, [event_queue]
    mov edx, 2048
    syscall

    test eax, eax
    jle exit_clean

    /* Initialize queue head = 0, tail = bytes read */
    mov dword ptr [ev_q_head], 0
    mov [ev_q_tail], eax

ev_pop_buffered:
    /* Capacity safety check: flush if out_buf > 1536 bytes to prevent overflow */
    lea rax, [out_buf + 1536]
    cmp r15, rax
    jb 1f
    call flush_out_buf
1:
    /* Zero-Copy Event Dispatch: read directly from [event_queue + rcx] in rsi! */
    mov ecx, [ev_q_head]
    lea rsi, [event_queue + rcx]
    add dword ptr [ev_q_head], 32

    movzx eax, byte ptr [rsi]
    and eax, 0x7F

    /* Direct O(1) 16-bit Relative Jump Table Dispatch (High Density, Cache Line Aligned) */
    cmp eax, 20
    ja event_loop
    lea rdx, [event_disp_base]
    movsx rax, word ptr [event_jump_table + rax*2]
    add rdx, rax
    jmp rdx

.align 2
event_jump_table:
    .short event_loop - event_disp_base         /* 0: Error */
    .short event_loop - event_disp_base         /* 1: Reply */
    .short handle_key - event_disp_base         /* 2: KeyPress */
    .short event_loop - event_disp_base         /* 3: KeyRelease */
    .short handle_btn_press - event_disp_base   /* 4: ButtonPress */
    .short handle_btn_release - event_disp_base /* 5: ButtonRelease */
    .short handle_motion - event_disp_base      /* 6: MotionNotify */
    .short handle_enter - event_disp_base       /* 7: EnterNotify */
    .short event_loop - event_disp_base         /* 8: LeaveNotify */
    .short event_loop - event_disp_base         /* 9: FocusIn */
    .short event_loop - event_disp_base         /* 10: FocusOut */
    .short event_loop - event_disp_base         /* 11: KeymapNotify */
    .short handle_expose - event_disp_base      /* 12: Expose */
    .short event_loop - event_disp_base         /* 13: GraphicsExpose */
    .short event_loop - event_disp_base         /* 14: NoExpose */
    .short event_loop - event_disp_base         /* 15: VisibilityNotify */
    .short event_loop - event_disp_base         /* 16: CreateNotify */
    .short handle_unmap - event_disp_base       /* 17: DestroyNotify */
    .short handle_unmap - event_disp_base       /* 18: UnmapNotify */
    .short event_loop - event_disp_base         /* 19: MapNotify */
    .short handle_map - event_disp_base         /* 20: MapRequest */
event_disp_base:

handle_btn_press:
    /*
     * ButtonPress Event:
     * [rsi + 1]: detail (button: 1 = Left, 3 = Right)
     * [rsi + 12]: event window
     * [rsi + 16]: child window (if clicked on subwindow/decoration)
     * [rsi + 20]: rootX (INT16)
     * [rsi + 22]: rootY (INT16)
     * [rsi + 24]: eventX (INT16)
     * [rsi + 26]: eventY (INT16)
     */
    mov edx, [rsi + 12]                 /* window */
    test edx, edx
    jz event_loop

    /* Read click position and calculate window geometry before ANY function call! */
    movsx ecx, word ptr [rsi + 20]      /* rootX */
    movsx r8d, word ptr [rsi + 22]      /* rootY */
    movsx r9d, word ptr [rsi + 24]      /* eventX */
    movsx r10d, word ptr [rsi + 26]     /* eventY */
    movzx eax, byte ptr [rsi + 1]       /* button */

    /* Save drag state */
    mov [drag_win], edx
    mov [drag_btn], eax
    mov [drag_off_x], r9d
    mov [drag_off_y], r10d
    mov [drag_start_x], ecx
    mov [drag_start_y], r8d
    mov qword ptr [drag_last_coords], -1

    /* Find window in active workspace BSP tree via O(1) global lookup */
    mov esi, edx
    call bsp_find_win_global
    test rax, rax
    jz event_loop                       /* Not a managed window */
    cmp ecx, r14d
    jne event_loop                      /* Not in active workspace */

    /* Update focus to clicked window */
    mov rbx, rax
    lea rax, [bsp_focuses]
    mov [rax + 8*r14], rbx
    mov rdi, rbx
    call bsp_update_history

    /*
     * Unified Atomic Batch:
     * 1. set_focus_buf (paints borders + SetInputFocus)
     * 2. X_GetGeometry (8 bytes)
     * 3. X_RaiseWindow (16 bytes)
     * All combined and flushed in ONE single sys_write!
     */
    mov edi, [drag_win]
    call set_focus_buf

    mov edx, [drag_win]
    mov rax, rdx
    shl rax, 32

    /* Packet 1: X_GetGeometry (8 bytes) */
    mov r8, rax
    or r8, (2 << 16) | 14               /* Opcode 14, len = 2 words */
    mov [r15 + 0], r8

    /* Packet 2: X_RaiseWindow (ConfigureWindow, stack mode = Above) (16 bytes) */
    or rax, (4 << 16) | 12              /* Opcode 12, len = 4 words */
    mov [r15 + 8], rax
    mov qword ptr [r15 + 16], 64        /* CWStackMode = Above */
    add r15, 24

    /* Flush focus + geometry query + raise in ONE syscall! */
    call flush_out_buf

    /* Read 32-byte X_GetGeometry reply directly into in_buf */
    xor eax, eax                        /* SYS_READ */
    mov edi, r13d
    lea rsi, [in_buf]
    push 32
    pop rdx
    syscall

    cmp byte ptr [in_buf], 1            /* Success reply? */
    jne event_loop

    movzx eax, word ptr [in_buf + 16]   /* width */
    movzx edx, word ptr [in_buf + 18]   /* height */
    mov [drag_orig_w], eax
    mov [drag_orig_h], edx

    /* Mark as floating (bit 5 in bpl) */
    or bpl, 32
    jmp event_loop

handle_motion:
    /*
     * MotionNotify during drag:
     * [rsi + 20]: rootX (INT16)
     * [rsi + 22]: rootY (INT16)
     */
    mov edx, [drag_win]
    test edx, edx
    jz event_loop

    /* Event Coalescing: compress consecutive MotionNotify events */
motion_coalesce_loop:
    mov eax, [ev_q_head]
    cmp eax, [ev_q_tail]
    jge motion_process
    movzx r11d, byte ptr [event_queue + rax]
    and r11b, 0x7F
    cmp r11b, 6
    jne motion_process
    lea rsi, [event_queue + rax]
    add dword ptr [ev_q_head], 32
    jmp motion_coalesce_loop

motion_process:
    movsx ecx, word ptr [rsi + 20]      /* current rootX */
    movsx r8d, word ptr [rsi + 22]      /* current rootY */

    cmp dword ptr [drag_btn], 3         /* Button 3 = Resize */
    je motion_resize

    /* Button 1 = Move: direct algebraic offset (new_x = cur_x - eventX) */
    mov eax, ecx
    sub eax, [drag_off_x]               /* new_x */

    mov esi, r8d
    sub esi, [drag_off_y]               /* new_y */

    /* Zero-Delta Motion Filter: drop redundant rendering frames (64-bit check) */
    shl rsi, 32
    mov eax, eax                        /* clear upper 32 bits of rax */
    or rax, rsi                         /* rax = (new_y << 32) | new_x */
    cmp rax, [drag_last_coords]
    je event_loop                       /* Exact duplicate frame: skip syscall & Xorg repaint! */
    mov [drag_last_coords], rax

    /* Precalculate 64-bit header + win_id */
    mov r8, rdx
    shl r8, 32
    or r8, (5 << 16) | 12               /* Opcode 12, len = 5 words */
    mov [r15 + 0], r8

    /* Naturally aligned stores: 64-bit aligned at offset 8, 32-bit aligned at offset 16 */
    mov ecx, eax                        /* ecx = new_x */
    shl rcx, 32
    or rcx, 3                           /* CWX | CWY = 3 */
    mov [r15 + 8], rcx                  /* 8-byte aligned store: mask + new_x */
    shr rax, 32                         /* eax = new_y */
    mov [r15 + 16], eax                 /* 4-byte aligned store: new_y */
    add r15, 20
    jmp event_loop

motion_resize:
    /* Branchless Clamping: MAX(50, orig + delta) via cmovl */
    mov r10d, 50
    mov eax, ecx
    sub eax, [drag_start_x]
    add eax, [drag_orig_w]
    cmp eax, 50
    cmovl eax, r10d

    mov esi, r8d
    sub esi, [drag_start_y]
    add esi, [drag_orig_h]
    cmp esi, 50
    cmovl esi, r10d

    /* Zero-Delta Motion Filter: drop redundant rendering frames (64-bit check) */
    shl rsi, 32
    mov eax, eax                        /* clear upper 32 bits of rax */
    or rax, rsi                         /* rax = (new_h << 32) | new_w */
    cmp rax, [drag_last_coords]
    je event_loop                       /* Exact duplicate frame: skip syscall & Xorg repaint! */
    mov [drag_last_coords], rax

    /* Precalculate 64-bit header + win_id */
    mov r8, rdx
    shl r8, 32
    or r8, (5 << 16) | 12               /* Opcode 12, len = 5 words */
    mov [r15 + 0], r8

    /* Naturally aligned stores: 64-bit aligned at offset 8, 32-bit aligned at offset 16 */
    mov ecx, eax                        /* ecx = new_w */
    shl rcx, 32
    or rcx, 12                          /* CWWidth | CWHeight = 12 */
    mov [r15 + 8], rcx                  /* 8-byte aligned store: mask + new_w */
    shr rax, 32                         /* eax = new_h */
    mov [r15 + 16], eax                 /* 4-byte aligned store: new_h */
    add r15, 20
    jmp event_loop

handle_btn_release:
    mov dword ptr [drag_win], 0
    mov qword ptr [drag_last_coords], -1
    jmp event_loop

handle_enter:
    /*
     * EnterNotify (Focus follows mouse):
     * [rsi + 1]: detail (2 = NotifyInferior)
     * [rsi + 12]: event window (uint32)
     * [rsi + 30]: mode (0 = NotifyNormal, 1 = NotifyGrab, 2 = NotifyUngrab, 3 = NotifyWhileGrabbed)
     */
    cmp byte ptr [rsi + 30], 0  /* mode == NotifyNormal */
    jne event_loop
    cmp byte ptr [rsi + 1], 2   /* detail != NotifyInferior */
    je event_loop

    mov edx, [rsi + 12]         /* edx = event window */
    test edx, edx
    jz event_loop

    /* Find leaf node with this window in active workspace via O(1) global lookup */
    mov esi, edx
    call bsp_find_win_global
    test rax, rax
    jz event_loop               /* Window not in active workspace BSP tree */
    cmp ecx, r14d
    jne event_loop              /* Window not in active workspace */

    /* Check if already focused */
    lea rbx, [bsp_focuses]
    cmp rax, [rbx + 8*r14]
    je event_loop               /* Already focused */

    /* Update focus */
    mov rdi, rax
    mov esi, r14d
    call activate_node_focus
    jmp event_loop

handle_expose:
    mov edx, [rsi + 4]          /* drawable/window */
    xor edi, edi                /* default target_mon = 0 */
    cmp edx, [bar_win + 0]
    je check_expose_count
    inc edi                     /* target_mon = 1 */
    cmp edx, [bar_win + 4]
    jne event_loop

check_expose_count:
    /*
     * Frame Optimization: Expose Compacting
     * In X11 wire protocol, bytes 16-17 of Expose event is `count` (uint16_t).
     * `count` indicates how many more Expose events for this window are queued.
     * If count > 0, discard intermediate frames and wait for count == 0!
     */
    movzx eax, word ptr [rsi + 16]
    test eax, eax
    jnz event_loop              /* More expose events incoming, don't redraw yet! */

    call draw_mon_bar
    jmp event_loop

/*
 * =========================================================================
 * KEY EVENT HANDLERS
 * =========================================================================
 */
handle_key:
    movzx ecx, byte ptr [rsi + 1]   /* keycode */
    movzx edx, word ptr [rsi + 28]  /* state/modifiers */

    /* O(1) 16-bit Relative Keycode Jump Table Dispatch (High Density, Cache Line Aligned) */
    sub ecx, 10
    cmp ecx, 55
    ja event_loop
    lea r11, [key_disp_base]
    movsx rax, word ptr [key_jump_table + rcx*2]
    add r11, rax
    jmp r11

.align 2
key_jump_table:
    .short key_ws_dispatch - key_disp_base   /* 0:  kc 10 (Alt+1) */
    .short key_ws_dispatch - key_disp_base   /* 1:  kc 11 (Alt+2) */
    .short key_ws_dispatch - key_disp_base   /* 2:  kc 12 (Alt+3) */
    .short key_ws_dispatch - key_disp_base   /* 3:  kc 13 (Alt+4) */
    .short key_ws_dispatch - key_disp_base   /* 4:  kc 14 (Alt+5) */
    .short key_ws_dispatch - key_disp_base   /* 5:  kc 15 (Alt+6) */
    .short key_ws_dispatch - key_disp_base   /* 6:  kc 16 (Alt+7) */
    .short key_ws_dispatch - key_disp_base   /* 7:  kc 17 (Alt+8) */
    .short key_ws_dispatch - key_disp_base   /* 8:  kc 18 (Alt+9) */
    .short event_loop - key_disp_base        /* 9:  kc 19 */
    .short event_loop - key_disp_base        /* 10: kc 20 */
    .short event_loop - key_disp_base        /* 11: kc 21 */
    .short event_loop - key_disp_base        /* 12: kc 22 */
    .short event_loop - key_disp_base        /* 13: kc 23 */
    .short key_q - key_disp_base             /* 14: kc 24 (Alt+q / Alt+Shift+q) */
    .short key_term - key_disp_base          /* 15: kc 25 (Alt+w) */
    .short event_loop - key_disp_base        /* 16: kc 26 */
    .short event_loop - key_disp_base        /* 17: kc 27 */
    .short event_loop - key_disp_base        /* 18: kc 28 */
    .short event_loop - key_disp_base        /* 19: kc 29 */
    .short event_loop - key_disp_base        /* 20: kc 30 */
    .short event_loop - key_disp_base        /* 21: kc 31 */
    .short event_loop - key_disp_base        /* 22: kc 32 */
    .short event_loop - key_disp_base        /* 23: kc 33 */
    .short event_loop - key_disp_base        /* 24: kc 34 */
    .short event_loop - key_disp_base        /* 25: kc 35 */
    .short key_term - key_disp_base          /* 26: kc 36 (Alt+Return) */
    .short event_loop - key_disp_base        /* 27: kc 37 */
    .short key_ws_prev - key_disp_base       /* 28: kc 38 (Alt+a) */
    .short key_ws_next - key_disp_base       /* 29: kc 39 (Alt+s) */
    .short key_menu - key_disp_base          /* 30: kc 40 (Alt+d) */
    .short key_f - key_disp_base             /* 31: kc 41 (Alt+f / Alt+Shift+f) */
    .short event_loop - key_disp_base        /* 32: kc 42 */
    .short key_hjkl_disp - key_disp_base     /* 33: kc 43 (Alt+h) */
    .short key_hjkl_disp - key_disp_base     /* 34: kc 44 (Alt+j) */
    .short key_hjkl_disp - key_disp_base     /* 35: kc 45 (Alt+k) */
    .short key_hjkl_disp - key_disp_base     /* 36: kc 46 (Alt+l) */
    .short event_loop - key_disp_base        /* 37: kc 47 */
    .short event_loop - key_disp_base        /* 38: kc 48 */
    .short event_loop - key_disp_base        /* 39: kc 49 */
    .short event_loop - key_disp_base        /* 40: kc 50 */
    .short event_loop - key_disp_base        /* 41: kc 51 */
    .short event_loop - key_disp_base        /* 42: kc 52 */
    .short event_loop - key_disp_base        /* 43: kc 53 */
    .short event_loop - key_disp_base        /* 44: kc 54 */
    .short key_split_h - key_disp_base       /* 45: kc 55 (Alt+v) */
    .short key_split_v - key_disp_base       /* 46: kc 56 (Alt+b) */
    .short event_loop - key_disp_base        /* 47: kc 57 */
    .short event_loop - key_disp_base        /* 48: kc 58 */
    .short event_loop - key_disp_base        /* 49: kc 59 */
    .short event_loop - key_disp_base        /* 50: kc 60 */
    .short event_loop - key_disp_base        /* 51: kc 61 */
    .short event_loop - key_disp_base        /* 52: kc 62 */
    .short event_loop - key_disp_base        /* 53: kc 63 */
    .short event_loop - key_disp_base        /* 54: kc 64 */
    .short key_floating - key_disp_base      /* 55: kc 65 (Alt+Space) */
key_disp_base:

key_ws_dispatch:
    test edx, 1                 /* ShiftMask = 1 ? */
    jnz key_move_to_ws
    mov edi, ecx
    call focus_workspace
    jmp event_loop

key_move_to_ws:
    mov edi, ecx
    call move_window_to_workspace
    jmp event_loop

key_q:
    test edx, 1                 /* ShiftMask = 1 */
    jnz exit_clean              /* Alt+Shift+q: Quit WM */
    jmp key_kill                /* Alt+q: Kill Active Window */

key_f:
    test edx, 1                 /* Shift pressed? */
    jnz key_floating            /* Alt+Shift+f: Floating Toggle */
    jmp key_fullscreen          /* Alt+f: Fullscreen Toggle */

/*
 * key_hjkl_disp:
 * Unified 2D Spatial Engine (Frameless, Zero Stack):
 * Fuses directional focus (Alt+HJKL) and directional window swap (Alt+Shift+HJKL).
 */
key_hjkl_disp:
    add ecx, 10                 /* restore keycode 43..46 */
    mov r10d, edx               /* r10d = state/modifiers */
    and r10d, 1                 /* bit 0 = ShiftMask */

    lea rbx, [bsp_focuses]
    mov rdi, [rbx + 8*r14]      /* rdi = current focused leaf */
    test rdi, rdi
    jz event_loop
    mov rbx, rdi                /* rbx = current focused leaf (callee-saved) */

    call hjkl_decode_dir        /* esi = orient, edx = dir */
    call bsp_node_in_direction  /* rax = neighbor leaf node */
    test rax, rax
    jz event_loop               /* No neighbor in that direction */
    mov r12, rax                /* r12 = target neighbor leaf node (callee-saved) */

    test r10d, r10d             /* Shift pressed? */
    jz hjkl_apply_focus         /* Alt+HJKL: navigate focus only */

    /* Alt+Shift+HJKL: Swap window IDs between rbx and r12 */
    mov r8d, [rbx + NODE_WIN]   /* r8d = current active window */
    mov r9d, [r12 + NODE_WIN]   /* r9d = neighbor window */
    mov [rbx + NODE_WIN], r9d
    mov [r12 + NODE_WIN], r8d   /* r12 now holds the active window */

    /* Update MRU and Hash Table for swapped windows */
    mov edi, r8d
    mov rsi, r12
    call win_cache_put
    mov edi, r9d
    mov rsi, rbx
    call win_cache_put

    /* Re-arrange grid to reflect new spatial layout */
    mov edi, r14d
    call bsp_arrange_ws_buf

hjkl_apply_focus:
    /* Update focus and tree history to target node (r12) */
    mov rdi, r12
    mov esi, r14d
    call activate_node_focus
    jmp event_loop

/*
 * hjkl_decode_dir(ecx = keycode 43..46)
 * Outputs:
 *   esi = orient (0 = Vertical, 1 = Horizontal)
 *   edx = dir (0 = Left/Up, 1 = Right/Down)
 */
hjkl_decode_dir:
    sub ecx, 43
    mov edx, ecx
    and edx, 1                  /* dir: 43(h)->0, 44(j)->1, 45(k)->0, 46(l)->1 */
    shr ecx, 1
    xor ecx, edx                /* orient: h->0, j->1, k->1, l->0 */
    mov esi, ecx
    ret

key_split_h:
    and bpl, ~6
    or bpl, 4                   /* bits 1..2 = 2 (horizontal) */
    jmp event_loop

key_split_v:
    and bpl, ~6
    or bpl, 2                   /* bits 1..2 = 1 (vertical) */
    jmp event_loop

key_ws_prev:
    /* Branchless Modulo Wrap Prev: if (cur_ws - 1 < 0) cur_ws = 8 */
    mov eax, r14d
    dec eax
    mov edx, WORKSPACE_COUNT - 1
    cmovs eax, edx
    mov edi, eax
    call focus_workspace
    jmp event_loop

key_ws_next:
    /* Branchless Modulo Wrap Next: if (cur_ws + 1 >= 9) cur_ws = 0 */
    mov eax, r14d
    inc eax
    xor edx, edx
    cmp eax, WORKSPACE_COUNT
    cmovge eax, edx
    mov edi, eax
    call focus_workspace
    jmp event_loop

key_menu:
    lea rsi, [sh_argv_menu]
    call spawn_cmd
    jmp event_loop

key_term:
    lea rsi, [sh_argv_term]
    call spawn_cmd
    jmp event_loop

key_kill:
    lea rbx, [bsp_focuses]
    mov rax, [rbx + 8*r14]
    test rax, rax
    jz event_loop
    mov edx, [rax + NODE_WIN]
    test edx, edx
    jz event_loop

    /*
     * Terminate client window and process tree (52 bytes batched in out_buf):
     * 1. X_SendEvent (Opcode 25, len=11 words = 44 bytes): sends ICCCM ClientMessage
     *    WM_PROTOCOLS / WM_DELETE_WINDOW so apps like Brave/Chrome cleanly shut down
     *    all background audio and renderer processes.
     * 2. X_KillClient (Opcode 113, len=2 words = 8 bytes): forces the X server to
     *    sever the client's socket connection so non-ICCCM apps die immediately too.
     */
    mov dword ptr [r15 + 0],  (11 << 16) | 25   /* X_SendEvent: propagate=0, len=11 */
    mov [r15 + 4],  edx                         /* destination = win */
    mov dword ptr [r15 + 8],  0                 /* event_mask = NoEventMask */
    mov dword ptr [r15 + 12], (32 << 8) | 33    /* ClientMessage (33), format=32 */
    mov [r15 + 16], edx                         /* window = win */
    mov eax, [atom_wm_proto]
    mov [r15 + 20], eax                         /* message_type = WM_PROTOCOLS */
    mov eax, [atom_wm_del]
    mov [r15 + 24], eax                         /* data.l[0] = WM_DELETE_WINDOW */
    mov qword ptr [r15 + 28], 0                 /* data.l[1..2] = 0 */
    mov qword ptr [r15 + 36], 0                 /* data.l[3..4] = 0 */

    /* Also append X_KillClient (8 bytes) */
    mov dword ptr [r15 + 44], (2 << 16) | 113
    mov [r15 + 48], edx
    add r15, 52
    jmp event_loop

/*
 * Window Presentation Engine:
 * Unified Fullscreen & Floating Mode Pipeline (Zero Stack, 128-bit SIMD Serializer)
 */
key_fullscreen:
    mov r10d, 1                 /* bit 0: is_fullscreen */
    jmp toggle_win_presentation

key_floating:
    mov r10d, 32                /* bit 5: is_floating */

toggle_win_presentation:
    lea rbx, [bsp_focuses]
    mov rax, [rbx + 8*r14]
    test rax, rax
    jz event_loop
    mov edx, [rax + NODE_WIN]
    test edx, edx
    jz event_loop

    xor bpl, r10b               /* Toggle presentation mode flag */
    test bpl, r10b
    jz win_restore_layout

    /* Set presentation on target monitor */
    mov edi, r14d
    call get_mon_for_ws         /* eax = monitor index */
    shl eax, 4                  /* 16 bytes per monitor */
    lea rbx, [mon_geom + rax]

    test r10b, 1                /* Fullscreen? */
    jz do_floating_calc

    /* Fullscreen: load monitor dimensions and zero border */
    movdqu xmm0, [rbx]          /* Load [mon_x, mon_y, mon_w, mon_h] in 1 SIMD load */
    xor eax, eax                /* border_width = 0 */
    jmp present_window_simd

do_floating_calc:
    /* Floating: 60% dimensions centered on target monitor */
    /* fw = (mon_w * 39322) >> 16 */
    mov eax, [rbx + 8]          /* mon_w */
    imul eax, 39322
    shr eax, 16                 /* eax = fw */
    mov r8d, eax                /* r8d = fw */

    /* fh = (mon_h * 39322) >> 16 */
    mov eax, [rbx + 12]         /* mon_h */
    imul eax, 39322
    shr eax, 16                 /* eax = fh */
    mov r9d, eax                /* r9d = fh */

    /* fx = mon_x + ((mon_w - fw) >> 1) */
    mov esi, [rbx + 8]          /* mon_w */
    sub esi, r8d
    sar esi, 1
    add esi, [rbx + 0]          /* esi = fx */

    /* fy = mon_y + ((mon_h - fh) >> 1) */
    mov edi, [rbx + 12]         /* mon_h */
    sub edi, r9d
    sar edi, 1
    add edi, [rbx + 4]          /* edi = fy */

    /* Vectorize 4 coordinates into xmm0 via SSE2 */
    movd xmm0, esi
    pinsrd xmm0, edi, 1
    pinsrd xmm0, r8d, 2
    pinsrd xmm0, r9d, 3
    mov eax, 1                  /* border_width = 1 */

/*
 * present_window_simd(edx = win_id, xmm0 = [x, y, w, h], eax = border_width, r15 = write cursor)
 * Emits 48 bytes (ConfigureWindow + X_RaiseWindow) via 128-bit SIMD burst.
 */
present_window_simd:
    /* Packet 1: ConfigureWindow (32 bytes) */
    mov r8, rdx
    shl r8, 32
    or r8, (8 << 16) | 12       /* Opcode 12, len = 8 words */
    mov [r15 + 0], r8
    mov dword ptr [r15 + 8], 31 /* CWX | CWY | CWWidth | CWHeight | CWBorderWidth */
    movdqu [r15 + 12], xmm0     /* SIMD 128-bit store: [x, y, w, h] written all at once! */
    mov [r15 + 28], eax         /* border_width */

    /* Packet 2: X_RaiseWindow (ConfigureWindow, stack mode = Above) (16 bytes) */
    mov r8, rdx
    shl r8, 32
    or r8, (4 << 16) | 12       /* Opcode 12, len = 4 words */
    mov [r15 + 32], r8
    mov qword ptr [r15 + 40], 64 /* CWStackMode = 64, Above = 0 in 1 quadword */
    add r15, 48
    jmp event_loop

win_restore_layout:
    mov edi, r14d
    call bsp_arrange_ws
    jmp event_loop

/*
 * activate_node_focus(rdi = leaf_node_ptr, esi = ws_idx)
 * Unified Window & Focus Activation Engine:
 * 1. Synchronizes bsp_focuses[ws_idx] = leaf_node_ptr
 * 2. If node != NULL: records branch history and sets active focus to node->win
 * 3. If node == NULL: clears surface and resets focus to root
 */
activate_node_focus:
    lea rax, [bsp_focuses]
    mov [rax + 8*rsi], rdi
    test rdi, rdi
    jz anf_root
    mov edx, [rdi + NODE_WIN]
    call bsp_update_history
    mov edi, edx
    jmp set_focus_buf
anf_root:
    xor edi, edi
    jmp set_focus_buf

set_focus:
    jmp set_focus_buf

.align 16
simd_border_inact_pkt:
    .long (4 << 16) | 2         /* Opcode 2: ChangeWindowAttributes (len=4) */
    .long 0                     /* Placeholder for win_id */
    .long 8                     /* CWBorderPixel */
    .long BORDER_INACTIVE       /* 0x2D2D2D */

.align 16
simd_border_act_pkt:
    .long (4 << 16) | 2         /* Opcode 2: ChangeWindowAttributes (len=4) */
    .long 0                     /* Placeholder for win_id */
    .long 8                     /* CWBorderPixel */
    .long BORDER_ACTIVE         /* 0x005577 */

/*
 * set_focus_buf(edi = win_id, r15 = write cursor)
 * Vectorized SIMD serialization: paints active/inactive borders and sets input focus.
 */
set_focus_buf:
    test edi, edi
    jnz sfb_has_win

    /* Clear focus: de-focus old window and give focus to root */
    mov eax, [last_focused_win]
    test eax, eax
    jz sfb_root_focus
    movdqa xmm0, [simd_border_inact_pkt]
    pinsrd xmm0, eax, 1
    movdqu [r15], xmm0
    add r15, 16
    mov dword ptr [last_focused_win], 0

sfb_root_focus:
    mov edx, [root_win]
    mov rax, rdx
    shl rax, 32
    or rax, (3 << 16) | (1 << 8) | 42  /* Opcode 42: SetInputFocus (revert_to=1) (12 bytes) */
    mov [r15 + 0], rax
    mov dword ptr [r15 + 8], 0

    /* Packet 2: X_ClearArea on root window (Opcode 61, exposures=1, w=0, h=0) (16 bytes) */
    mov dword ptr [r15 + 12], (4 << 16) | (1 << 8) | 61
    mov [r15 + 16], edx
    mov qword ptr [r15 + 20], 0
    add r15, 28
    ret

sfb_has_win:
    mov eax, [last_focused_win]
    cmp eax, edi
    je sfb_do_input_focus       /* Already has active border */

    /* If old window exists, paint border_inactive (0x2D2D2D) via 128-bit vector burst */
    test eax, eax
    jz sfb_paint_active
    movdqa xmm0, [simd_border_inact_pkt]
    pinsrd xmm0, eax, 1
    movdqu [r15], xmm0
    add r15, 16

sfb_paint_active:
    /* Paint new window border_active (0x005577) via 128-bit vector burst */
    movdqa xmm0, [simd_border_act_pkt]
    pinsrd xmm0, edi, 1
    movdqu [r15], xmm0
    add r15, 16

    mov [last_focused_win], edi

sfb_do_input_focus:
    mov eax, edi
    shl rax, 32
    or rax, (3 << 16) | (1 << 8) | 42
    mov [r15 + 0], rax
    mov dword ptr [r15 + 8], 0
    add r15, 12
focus_ret:
    ret

/*
 * flush_out_buf(r15 = write cursor at end of out_buf)
 * Sends all bytes accumulated in [out_buf .. r15) in ONE single sys_write!
 * Automatically resets r15 to out_buf.
 */
flush_out_buf:
    lea rsi, [out_buf]
    mov rdx, r15
    sub rdx, rsi
    jle flush_reset
    push SYS_WRITE
    pop rax
    mov edi, r13d
    syscall
flush_reset:
    lea r15, [out_buf]
    ret

/*
 * spawn_cmd(rsi = argv_pointer)
 * Uses SYS_VFORK (58) for the inner child to avoid page-table copying before execve!
 */
spawn_cmd:
    push rsi
    push SYS_FORK
    pop rax
    syscall
    test eax, eax
    jnz parent_fork

    mov al, SYS_VFORK           /* eax == 0 in child! */
    syscall
    test eax, eax
    jnz child_exit

    mov al, SYS_CLOSE           /* eax == 0 in grandchild! */
    mov edi, r13d
    syscall

    pop rsi
    mov al, SYS_EXECVE          /* eax == 0 after close! */
    lea rdi, [sh_bin]
    mov rdx, [saved_envp]
    syscall

child_exit:
    push SYS_EXIT
    pop rax
    xor edi, edi
    syscall

parent_fork:
    pop rsi
    mov edi, eax
    push SYS_WAIT4
    pop rax
    xor esi, esi
    xor edx, edx
    xor r10d, r10d
    syscall
    ret

/*
 * =========================================================================
 * WORKSPACE MANAGEMENT (9 WORKSPACES)
 * =========================================================================
 */

/*
 * focus_workspace(edi = target_ws 0..8)
 * Dual-monitor aware:
 * - Identifies target monitor (m) for target_ws.
 * - Retrieves old_ws visible on that monitor (mon_cur_ws[m]).
 * - If old_ws != target_ws, hides old_ws UNLESS displayed on the other monitor.
 * - Updates mon_cur_ws[m] = target_ws, r14d = target_ws.
 * - Batches hide + status bar update + arrangement + focus in ONE atomic sys_write!
 */
focus_workspace:
    cmp edi, r14d
    je fw_done

    mov r12d, edi               /* r12d = new_ws */

    /* Determine target monitor m (0 or 1) */
    mov edi, r12d
    call get_mon_for_ws         /* eax = m */
    mov r10d, eax               /* r10d = target monitor m */

    /* Get old workspace currently shown on target monitor m */
    movzx ebx, byte ptr [mon_cur_ws + r10]  /* ebx = old_ws on monitor m */
    cmp ebx, r12d
    je fw_same_ws

    /* Update visible workspace on target monitor */
    mov [mon_cur_ws + r10], r12b

    /*
     * Zero-Flicker Workspace Transition:
     * 1. Arrange new workspace windows FIRST on-screen (covering previous windows).
     * 2. Move old workspace windows off-screen (-9999, -9999) SECOND (eliminating screen blanking!).
     * 3. Update status bar and focus THIRD, all committed in ONE atomic sys_write!
     */
    mov edi, r12d
    call bsp_arrange_ws_buf

    /* Check if old_ws is displayed on the other monitor (in dual monitor mode) */
    test bpl, 8                 /* bit 3 = is_dual_mon */
    jz fw_do_hide               /* Single monitor: always hide old_ws */

    mov eax, r10d
    xor eax, 1                  /* other monitor = m ^ 1 */
    movzx eax, byte ptr [mon_cur_ws + rax]
    cmp eax, ebx
    je fw_bar_and_focus         /* Other monitor is viewing old_ws: do NOT hide! */

fw_do_hide:
    call hide_ws_windows
    jmp fw_bar_and_focus

fw_same_ws:
    mov edi, r12d
    call bsp_arrange_ws_buf

fw_bar_and_focus:
    /* Update status bar: redraw old_ws (ebx) as inactive and new_ws (r12d) as active */
    mov r8d, r14d               /* r8d = previous globally active workspace */
    mov r14d, r12d              /* r14d = newly active workspace */
    mov edi, r8d
    call draw_single_ws_buf
    mov edi, r12d
    call draw_single_ws_buf
    cmp ebx, r8d
    je fw_focus
    cmp ebx, r12d
    je fw_focus
    mov edi, ebx
    call draw_single_ws_buf

fw_focus:
    /* Focus active window on new workspace (or clear focus to root if empty) */
    lea rax, [bsp_focuses]
    mov rdi, [rax + 8*r12]
    mov esi, r12d
    jmp activate_node_focus

fw_done:
    ret

/*
 * move_window_to_workspace(edi = target_ws 0..8)
 * Batches hiding the moved window + rearranging the current workspace in out_buf!
 */
move_window_to_workspace:
    cmp edi, r14d
    je mw_done

    mov r12d, edi               /* target_ws */

    /* Get current focused leaf */
    lea rax, [bsp_focuses]
    mov rdi, [rax + 8*r14]
    test rdi, rdi
    jz mw_done

    mov ebx, [rdi + NODE_WIN]   /* ebx = win */
    test ebx, ebx
    jz mw_done

    /* Detach from current workspace (rdi = leaf_node) */
    call bsp_tree_detach

    /* Batch hide window + re-arrange current workspace */
    movdqa xmm0, [simd_hide_pkt]
    pinsrd xmm0, ebx, 1
    movdqu [r15 + 0], xmm0
    mov dword ptr [r15 + 16], -9999
    add r15, 20

    mov edi, r14d
    call bsp_arrange_ws_buf

    /* Insert window into target workspace tree */
    mov edi, ebx
    mov esi, r12d
    call bsp_tree_insert_into_ws

mw_done:
    ret

.align 16
simd_hide_pkt:
    .long (5 << 16) | 12        /* Opcode 12: ConfigureWindow (len=5) */
    .long 0                     /* Placeholder for win_id */
    .long 3                     /* CWX | CWY = 3 */
    .long -9999                 /* x = -9999 (off-screen) */

/*
 * hide_ws_windows(ebx = ws_idx, r15 = write cursor):
 * Flat L1 Cache Scan (Zero Stack, Zero Recursion):
 * Scans node_pool and emits 20-byte off-screen ConfigureWindow packets for all active windows in ws_idx.
 */
hide_ws_windows:
    mov ecx, [node_idx]
    test ecx, ecx
    jz hww_done
    lea rdx, [node_pool]

hww_loop:
    mov eax, [rdx + NODE_WIN]
    test eax, eax
    jz hww_next
    movzx esi, byte ptr [rdx + NODE_INFO]
    shr esi, 1
    and esi, 0xF
    cmp esi, ebx
    jne hww_next

    /* Window belongs to target workspace: serialize 20-byte off-screen packet */
    movdqa xmm0, [simd_hide_pkt]
    pinsrd xmm0, eax, 1
    movdqu [r15 + 0], xmm0
    mov dword ptr [r15 + 16], -9999
    add r15, 20

hww_next:
    add rdx, 32
    dec ecx
    jnz hww_loop

hww_done:
    ret

/*
 * =========================================================================
 * TOP STATUS BAR RENDERING (Pure X11 Wire Protocol)
 * Supports Independent Bars per Monitor!
 * =========================================================================
 */
init_bar:
    lea r15, [out_buf]                          /* r15 = unified batch write cursor */
    xor r12d, r12d                              /* r12d = monitor index (0 or 1) */

create_bar_loop:
    mov eax, r12d
    shl eax, 4                                  /* r12d * 16 bytes mon_geom */
    lea rbx, [mon_geom]
    add rbx, rax

    /* 1. X_CreateWindow (Opcode 1, length 11 words = 44 bytes) */
    mov dword ptr [r15 + 0], (11 << 16) | 1
    mov eax, [bar_win + 4*r12]
    mov [r15 + 4], eax                          /* wid */
    mov eax, [root_win]
    mov [r15 + 8], eax                          /* parent */

    mov eax, [rbx + 0]
    add eax, GAP_PX                             /* x */
    mov edx, [rbx + 4]
    add edx, GAP_PX                             /* y */
    shl edx, 16
    and eax, 0xFFFF
    or eax, edx
    mov [r15 + 12], eax                         /* x, y */

    mov eax, [rbx + 8]
    sub eax, GAP_PX * 2                         /* w */
    mov edx, BAR_HEIGHT                         /* h */
    shl edx, 16
    and eax, 0xFFFF
    or eax, edx
    mov [r15 + 16], eax                         /* w, h */

    mov dword ptr [r15 + 20], (1 << 16) | 0     /* border_width = 0, class = InputOutput (1) */
    mov dword ptr [r15 + 24], 0                 /* visual = CopyFromParent (0) */
    mov dword ptr [r15 + 28], 0x00000a02        /* CWBackPixel | CWOverrideRedirect | CWEventMask */
    mov dword ptr [r15 + 32], BG_COLOR          /* background pixel (#000000) */
    mov dword ptr [r15 + 36], 1                 /* override_redirect = True */
    mov dword ptr [r15 + 40], 0x00008000        /* ExposureMask */

    /* 2. X_MapWindow (Opcode 8, len 2 words = 8 bytes) */
    mov dword ptr [r15 + 44], (2 << 16) | 8
    mov eax, [bar_win + 4*r12]
    mov [r15 + 48], eax
    add r15, 52

    inc r12d
    cmp r12d, 1
    jne create_bar_done
    test bpl, 8                                 /* bit 3 = is_dual_mon */
    jnz create_bar_loop
create_bar_done:

    /* 3. Pre-create 2 static GCs for X_ImageText8 (Eliminates PolyFillRectangle AND ChangeGC!):
       - bar_gc + 0: active workspace digit GC (fg = 0xc2c2b0, bg = #000000)
       - bar_gc + 1: inactive workspace digit GC (fg = 0x685742, bg = #000000)
     */
    mov ecx, [bar_gc]
    mov edx, [bar_win + 0]

    /* GC 0: Active text + background cell (24 bytes) */
    mov dword ptr [r15 + 0], (6 << 16) | 55
    mov [r15 + 4], ecx
    mov [r15 + 8], edx
    mov dword ptr [r15 + 12], 12                /* GCForeground (4) | GCBackground (8) */
    mov dword ptr [r15 + 16], 0xc2c2b0          /* fg = active cream/gold */
    mov dword ptr [r15 + 20], BG_COLOR          /* bg = bar pure black background (#000000) */

    /* GC 1: Inactive text + background cell (24 bytes) */
    inc ecx
    mov dword ptr [r15 + 24], (6 << 16) | 55
    mov [r15 + 28], ecx
    mov [r15 + 32], edx
    mov dword ptr [r15 + 36], 12
    mov dword ptr [r15 + 40], 0x685742          /* fg = inactive brown/gray */
    mov dword ptr [r15 + 44], BG_COLOR          /* bg = bar pure black background (#000000) */

    /* 4. SelectInput and set background color on root window (20 bytes) */
    mov dword ptr [r15 + 48], (5 << 16) | 2     /* Opcode 2: ChangeWindowAttributes (len=5) */
    mov eax, [root_win]
    mov [r15 + 52], eax
    mov dword ptr [r15 + 56], 0x00000802        /* CWBackPixel (0x2) | CWEventMask (0x800) */
    mov dword ptr [r15 + 60], BG_COLOR          /* CWBackPixel value: root desktop background (#000000) */
    mov dword ptr [r15 + 64], 0x00180001        /* SubstructureRedirect | SubstructureNotify | KeyPress */
    add r15, 68

    /* 5. Append initial bar render into [r15] */
    call draw_all_bars_buf

    /* 6. Append all 72 X_GrabKey requests (1152 bytes) into [r15] and flush in ONE syscall! */
    lea rbx, [shortcut_keys]
    mov r12d, [root_win]

grab_batch_loop:
    movzx edi, byte ptr [rbx]                   /* keycode */
    test edi, edi
    jz grab_batch_send
    movzx esi, byte ptr [rbx + 1]               /* modifiers */

    /* Precalculate 64-bit header + win_id */
    mov rax, r12
    shl rax, 32
    or rax, (4 << 16) | (1 << 8) | 33           /* Opcode 33: X_GrabKey (len=4, owner_events=1) */
    mov [r15 + 0], rax
    mov [r15 + 16], rax

    /* Pack (modifiers | (keycode << 16) | (pointer_mode << 24)) */
    mov edx, edi
    shl edx, 16
    and esi, 0xFFFF
    or edx, esi
    or edx, (1 << 24)                           /* pointer_mode = 1 (Async) */

    /* Pack edx + keyboard_mode (1) into 64-bit */
    mov rax, 1                                  /* keyboard_mode = 1 (Async) */
    shl rax, 32
    mov edx, edx
    or rax, rdx
    mov [r15 + 8], rax                          /* standard combo */

    /* NumLock/CapsLock combo: modifiers | 0x10 */
    or rax, 0x10
    mov [r15 + 24], rax                         /* NumLock combo */

    add r15, 32
    add rbx, 2
    jmp grab_batch_loop

grab_batch_send:
    call flush_out_buf
    ret

/*
 * draw_mon_bar(edi = target_mon 0 or 1):
 * Unified Per-Monitor Bar Redraw Engine (Frameless, Zero Stack):
 * Draws workspaces filtered by monitor (or all if target_mon == -1 / single monitor).
 */
draw_all_bars:
draw_all_bars_buf:
    mov r8d, -1                 /* -1 = draw all workspaces */
    jmp dmb_entry

draw_mon_bar:
    mov r8d, edi                /* target_mon (0 or 1) */
    test bpl, 8                 /* is_dual_mon */
    jnz dmb_entry
    mov r8d, -1                 /* single monitor: draw all */

dmb_entry:
    xor edi, edi
dmb_loop:
    cmp r8d, -1
    je dmb_draw
    movzx eax, byte ptr [ws_mon_table + rdi]
    and eax, 1
    cmp eax, r8d
    jne dmb_next
dmb_draw:
    call draw_single_ws_buf
dmb_next:
    inc edi
    cmp edi, WORKSPACE_COUNT
    jl dmb_loop
    ret

/*
 * draw_single_ws_buf(edi = ws_idx 0..8, r15 = write cursor):
 * Surgical O(1) Frameless Single-Digit Bar Renderer via X_ImageText8 (Opcode 76, 20 bytes):
 * Zero stack frame, lea-shift multiplication, bit-slices monitor & X offset.
 */
draw_single_ws_buf:
    xor ecx, ecx                                /* default single-monitor: mon_idx = 0 */
    lea esi, [rdi + rdi*2]
    shl esi, 3
    add esi, 10                                 /* default single-monitor X = 10 + ws_idx * 24 */

    test bpl, 8                                 /* bit 3 = is_dual_mon */
    jz dsw_emit

    movzx ecx, byte ptr [ws_mon_table + rdi]    /* 1-byte packed load: (X & ~1) | mon_idx */
    mov esi, ecx
    and ecx, 1                                  /* bit 0 = mon_idx (0 or 1) */
    and esi, 0xFE                               /* bits 1..7 = precomputed even X coordinate! */

dsw_emit:
    /* Branchless GC selection: bar_gc + (edi != cur_ws) */
    mov edx, [bar_gc]
    cmp edi, r14d
    setne al
    movzx eax, al
    add edx, eax

    /* 64-bit Store 1: Header (0x0005014c) + bar_win[rcx] (low 32b = header, high 32b = win_id) */
    mov eax, [bar_win + 4*rcx]
    shl rax, 32
    or rax, (5 << 16) | (1 << 8) | 76
    mov [r15 + 0], rax

    /* 64-bit Store 2: GC ID + (Y=13 | X) (low 32b = GC, high 32b = (13 << 16) | X) */
    or esi, (13 << 16)
    mov rax, rsi
    shl rax, 32
    mov edx, edx
    or rax, rdx
    mov [r15 + 8], rax

    /* 32-bit Store 3: Character byte '1' + edi */
    lea eax, [rdi + '1']
    mov [r15 + 16], eax

    add r15, 20
    ret

/*
 * =========================================================================
 * DUAL MONITOR DETECTION VIA XINERAMA EXTENSION
 * =========================================================================
 */
detect_monitors:
    /* Default: 1 monitor = full screen (wm_state bit 3 starts as 0) */
    mov eax, [screen_w]
    mov edx, [screen_h]
    mov dword ptr [mon_geom + 0],  0   /* x */
    mov dword ptr [mon_geom + 4],  0   /* y */
    mov [mon_geom + 8],  eax           /* w */
    mov [mon_geom + 12], edx           /* h */

    /* QueryExtension "XINERAMA" (Opcode 98, len 4 words) */
    mov dword ptr [out_buf + 0], (4 << 16) | 98
    mov dword ptr [out_buf + 4], 8
    mov rax, 0x414d4152454e4958
    mov [out_buf + 8], rax /* "XINERAMA" */

    push SYS_WRITE
    pop rax
    mov edi, r13d
    lea rsi, [out_buf]
    push 16
    pop rdx
    syscall

    /* Read reply (32 bytes) - rdi still == r13d, edx was 16 (< 256)! */
    xor eax, eax                /* SYS_READ = 0 */
    lea rsi, [in_buf]
    mov dl, 32
    syscall

    cmp byte ptr [in_buf], 1            /* Success? */
    jne mon_detect_done
    cmp byte ptr [in_buf + 8], 0        /* present? */
    je mon_detect_done

    movzx r12d, byte ptr [in_buf + 9]    /* opcode base */

    /* Send XineramaQueryScreens (opcode, minor 5, len 1 word) */
    /* byte 0 = opcode (ebx), byte 1 = minor (5), byte 2..3 = len (1) */
    mov eax, (1 << 16) | (5 << 8)
    or eax, r12d
    mov [out_buf + 0], eax

    push SYS_WRITE
    pop rax
    lea rsi, [out_buf]
    mov dl, 4                   /* rdi still == r13d, edx was 32 (< 256)! */
    syscall

    /* Read reply header (32 bytes) - rdi still == r13d, edx was 4 (< 256)! */
    xor eax, eax                /* SYS_READ = 0 */
    lea rsi, [in_buf]
    mov dl, 32
    syscall

    cmp byte ptr [in_buf], 1
    jne mon_detect_done

    mov ecx, [in_buf + 8]               /* number of screens */
    cmp ecx, 2
    jge mon_read_screens

    /* If num_screens == 1, drain its 8-byte structure so atom replies aren't corrupted */
    test ecx, ecx
    jz mon_detect_done
    xor eax, eax                /* SYS_READ = 0 */
    lea rsi, [in_buf]
    push 8
    pop rdx
    syscall
    jmp mon_detect_done

mon_read_screens:
    /* 2 monitors detected! Read 2 x 8 bytes monitor structures (rdi == r13d) with full drain */
    xor ebx, ebx                /* total bytes read */
mon_read_loop:
    xor eax, eax                /* SYS_READ = 0 */
    lea rsi, [in_buf + rbx]
    mov edx, 16
    sub edx, ebx
    syscall
    test eax, eax
    jle mon_detect_done
    add ebx, eax
    cmp ebx, 16
    jl mon_read_loop

    /* Monitor 0 */
    movsx eax, word ptr [in_buf + 0]    /* x_org */
    movsx edx, word ptr [in_buf + 2]    /* y_org */
    movzx ecx, word ptr [in_buf + 4]    /* width */
    movzx r8d, word ptr [in_buf + 6]    /* height */
    mov [mon_geom + 0],  eax
    mov [mon_geom + 4],  edx
    mov [mon_geom + 8],  ecx
    mov [mon_geom + 12], r8d

    /* Monitor 1 */
    movsx eax, word ptr [in_buf + 8]    /* x_org */
    movsx edx, word ptr [in_buf + 10]   /* y_org */
    movzx ecx, word ptr [in_buf + 12]   /* width */
    movzx r8d, word ptr [in_buf + 14]   /* height */
    mov [mon_geom + 16], eax
    mov [mon_geom + 20], edx
    mov [mon_geom + 24], ecx
    mov [mon_geom + 28], r8d

    or bpl, 8                           /* Set bit 3 (is_dual_mon = 1) */
    mov byte ptr [mon_cur_ws + 0], 8    /* Monitor 0 starts displaying Workspace 9 (idx 8) */
    mov byte ptr [mon_cur_ws + 1], 0    /* Monitor 1 starts displaying Workspace 1 (idx 0) */

mon_detect_done:
    /*
     * Intern ICCCM atoms in ONE pipelined batch (44 bytes total):
     * 1. X_InternAtom "WM_PROTOCOLS" (20 bytes)
     * 2. X_InternAtom "WM_DELETE_WINDOW" (24 bytes)
     */
    mov dword ptr [out_buf + 0],  (5 << 16) | 16    /* Opcode 16, only_if_exists=0, len=5 */
    mov dword ptr [out_buf + 4],  12                /* name_len = 12 */
    mov rax, 0x4f544f52505f4d57                     /* "WM_PROTO" */
    mov [out_buf + 8], rax
    mov dword ptr [out_buf + 16], 0x534c4f43        /* "COLS" */

    mov dword ptr [out_buf + 20], (6 << 16) | 16    /* Opcode 16, only_if_exists=0, len=6 */
    mov dword ptr [out_buf + 24], 16                /* name_len = 16 */
    mov rax, 0x54454c45445f4d57                     /* "WM_DELET" */
    mov [out_buf + 28], rax
    mov rax, 0x574f444e49575f45                     /* "E_WINDOW" */
    mov [out_buf + 36], rax

    push SYS_WRITE
    pop rax
    lea rsi, [out_buf]
    push 44                     /* rdi still == r13d! */
    pop rdx
    syscall

    /* Read both 32-byte replies (64 bytes total) - rdi == r13d, edx was 44 (< 256)! */
    xor eax, eax                /* SYS_READ = 0 */
    lea rsi, [in_buf]
    mov dl, 64
    syscall

    mov eax, [in_buf + 8]
    mov [atom_wm_proto], eax
    mov eax, [in_buf + 40]
    mov [atom_wm_del], eax

    ret

/*
 * get_mon_for_ws(edi = ws_idx 0..8) -> eax = monitor index (0 or 1)
 */
get_mon_for_ws:
    test bpl, 8                         /* bit 3 = is_dual_mon */
    jz single_mon
    cmp edi, WORKSPACE_COUNT
    jge single_mon
    movzx eax, byte ptr [ws_mon_table + rdi]
    and eax, 1                          /* Extract bit 0 (monitor 0 or 1) */
    ret
single_mon:
    xor eax, eax
    ret

.align 16
simd_map_attr_pkt:
    .long (4 << 16) | 2         /* Opcode 2: ChangeWindowAttributes (len=4) */
    .long 0                     /* Placeholder for win_id */
    .long 0x00000800            /* CWEventMask */
    .long 0x00000010            /* EnterWindowMask */

.align 16
simd_grab_hdr_pkt:
    .long (6 << 16) | 28        /* Opcode 28: X_GrabButton (len=6) */
    .long 0                     /* Placeholder for win_id */
    .long (1 << 24) | (1 << 16) | 0x4C  /* owner_events=1, pointer_mode=1, event_mask=0x4C */
    .long 0                     /* confine_to=None, cursor=None */

/*
 * =========================================================================
 * TRUE HIERARCHICAL BSP TREE ENGINE
 * =========================================================================
 */

handle_map:
    mov edx, [rsi + 8]
    test edx, edx
    jz event_loop
    mov r12d, edx

    /* Check if window is already managed in any workspace via O(1) global lookup */
    mov esi, r12d
    call bsp_find_win_global
    test rax, rax
    jnz event_loop              /* Already managed: ignore duplicate MapRequest! */

    /* Insert into active workspace BSP tree */
    mov edi, r12d
    mov esi, r14d
    call bsp_tree_insert_into_ws

    /* Zero-Flicker Map Pipeline:
       1. ConfigureWindow (bsp_arrange_ws_buf) FIRST while new window is still unmapped!
       2. ChangeWindowAttributes (Select EnterWindowMask = 0x10) so focus-follows-mouse works!
       3. MapWindow SECOND so window appears directly at its final tiled geometry!
       4. SetInputFocus THIRD, all flushed in ONE atomic sys_write! */
    mov edi, r14d
    call bsp_arrange_ws_buf

    /*
     * SIMD 128-bit Vectorized Window Mapping Pipeline (120 bytes):
     * 1. ChangeWindowAttributes (16 bytes, 1 SIMD store)
     * 2. 4x X_GrabButton (96 bytes, 4 SIMD stores + 4 quadword stores)
     * 3. X_MapWindow (8 bytes, 1 quadword store)
     */
    movdqa xmm0, [simd_map_attr_pkt]
    pinsrd xmm0, r12d, 1
    movdqu [r15 + 0], xmm0

    movdqa xmm1, [simd_grab_hdr_pkt]
    pinsrd xmm1, r12d, 1

    /* Button 1 + Mod1 (0x08) */
    movdqu [r15 + 16], xmm1
    mov rcx, 0x0008000100000000
    mov [r15 + 32], rcx

    /* Button 1 + Mod1 | NumLock (0x18) */
    movdqu [r15 + 40], xmm1
    mov rdx, 0x0018000100000000
    mov [r15 + 56], rdx

    /* Button 3 + Mod1 (0x08) */
    movdqu [r15 + 64], xmm1
    mov rcx, 0x0008000300000000
    mov [r15 + 80], rcx

    /* Button 3 + Mod1 | NumLock (0x18) */
    movdqu [r15 + 88], xmm1
    mov rdx, 0x0018000300000000
    mov [r15 + 104], rdx

    /* X_MapWindow (8 bytes) */
    mov rax, r12
    shl rax, 32
    or rax, (2 << 16) | 8
    mov [r15 + 112], rax

    add r15, 120

    mov edi, r12d
    call set_focus_buf
    jmp event_loop

handle_unmap:
    mov edx, [rsi + 8]
    test edx, edx
    jz event_loop

    /* O(1) Global Window Lookup */
    mov esi, edx
    call bsp_find_win_global
    test rax, rax
    jz event_loop               /* Window not managed by fiwm */

found_unmap_node:
    /* Detach node from its workspace */
    mov r12d, ecx               /* r12d = workspace index (0..8) */
    mov rdi, rax
    call bsp_tree_detach

    /* If it was in active workspace, re-arrange and focus in ONE atomic batch */
    cmp r12d, r14d
    jne event_loop

    lea rax, [bsp_roots]
    mov rdi, [rax + 8*r14]
    call bsp_first_leaf
    mov rdi, rax
    mov esi, r14d
    call activate_node_focus
    mov edi, r14d
    call bsp_arrange_ws_buf
    jmp event_loop

/*
 * bsp_tree_insert_into_ws(edi = win, esi = ws_idx)
 */
bsp_tree_insert_into_ws:
    mov r8d, esi               /* ws_idx */

    call alloc_node
    mov [rax + NODE_WIN], edi
    lea edx, [r8 + r8]         /* edx = ws_idx << 1 */
    mov [rax + NODE_INFO], dl  /* packed NODE_INFO: (ws_idx << 1) | 0 */
    mov r9, rax                /* r9 = new leaf */

    lea rax, [bsp_roots]
    mov r10, [rax + 8*r8]
    test r10, r10
    jnz ws_split_leaf

    /* First window on this workspace becomes root */
    lea rax, [bsp_roots]
    mov [rax + 8*r8], r9
    jmp ws_insert_done

ws_split_leaf:
    lea rax, [bsp_focuses]
    mov r10, [rax + 8*r8]
    test r10, r10
    jnz ws_has_focus
    lea rax, [bsp_roots]
    mov rdi, [rax + 8*r8]
    call bsp_first_leaf
    mov r10, rax
ws_has_focus:
    call alloc_node
    mov r11, rax               /* r11 = new split node */

    /* Determine split direction from bpl bits 1..2: 1 = V, 2 = H, 0 = auto */
    mov ecx, ebp
    shr cl, 1
    and cl, 3
    jz ws_auto_split

    /* Explicit split chosen by user (Alt+v or Alt+b) */
    dec cl                      /* 1 -> 0 (V), 2 -> 1 (H) */
    and bpl, ~6                 /* clear next_split_sel bits 1..2 */
    jmp ws_apply_split

ws_auto_split:
    /* Alternate split: invert parent's split bit (or 0 if root) */
    xor ecx, ecx
    mov rax, [r10 + NODE_PARENT]
    test rax, rax
    jz ws_apply_split
    movzx ecx, byte ptr [rax + NODE_INFO]
    xor ecx, 1
    and ecx, 1

ws_apply_split:
    lea edx, [r8 + r8]          /* ws_idx << 1 */
    or cl, dl                   /* pack (ws_idx << 1) | split_type into 1 byte! */
    mov [r11 + NODE_INFO], cl
    mov rax, [r10 + NODE_PARENT]
    mov [r11 + NODE_PARENT], rax

    mov [r11 + NODE_FIRST], r10
    mov [r11 + NODE_SECOND], r9
    mov [r10 + NODE_PARENT], r11
    mov [r9 + NODE_PARENT], r11

    /* Branchless parent / root slot selection */
    lea rcx, [bsp_roots]
    lea rcx, [rcx + 8*r8]       /* Default target: &bsp_roots[ws_idx] */

    test rax, rax
    jz ws_link_done             /* If no parent, r11 becomes new root */

    /* Parent exists: select first or second child slot branchlessly via cmove */
    lea rdx, [rax + NODE_FIRST]
    lea rcx, [rax + NODE_SECOND]
    cmp [rdx], r10
    cmove rcx, rdx              /* If first == r10, rcx = &parent->first, else &parent->second */

ws_link_done:
    mov [rcx], r11              /* Link new split node into parent or root slot */

ws_insert_done:
    lea rax, [bsp_focuses]
    mov [rax + 8*r8], r9
    mov rdi, r9
    call bsp_update_history

    /* Eager O(1) Cache Registration */
    mov edi, [r9 + NODE_WIN]
    mov rsi, r9
    jmp win_cache_put           /* Tail call */

/*
 * bsp_tree_detach(rdi = leaf_node)
 */
bsp_tree_detach:
    mov r8, rdi
    movzx r9d, byte ptr [r8 + NODE_INFO]
    shr r9d, 1                              /* Extract ws_idx = NODE_INFO >> 1 */

    /* Eager Invalidation from MRU and Hash Table */
    mov edi, [r8 + NODE_WIN]
    call win_cache_evict
    mov dword ptr [r8 + NODE_WIN], 0    /* Clear window ID so free/detached nodes never match linear scan */

    mov r10, [r8 + NODE_PARENT]
    test r10, r10
    jnz td_has_parent

    /* Removing root: push leaf node (r8) onto O(1) free-list! */
    mov rax, [free_node_head]
    mov [r8 + NODE_FIRST], rax
    mov [free_node_head], r8

    lea rax, [bsp_roots]
    mov qword ptr [rax + 8*r9], 0
    lea rax, [bsp_focuses]
    mov qword ptr [rax + 8*r9], 0
    ret

td_has_parent:
    /* Branchless sibling selection: if (first == r8) sibling = second */
    mov r11, [r10 + NODE_FIRST]
    mov rax, [r10 + NODE_SECOND]
    cmp r11, r8
    cmove r11, rax

    mov rax, [r10 + NODE_PARENT]
    mov [r11 + NODE_PARENT], rax

    /* Branchless grandparent / root slot selection */
    lea rcx, [bsp_roots]
    lea rcx, [rcx + 8*r9]       /* Default target: &bsp_roots[ws_idx] */

    test rax, rax
    jz td_store_sib             /* If no grandparent, update bsp_roots directly */

    /* Grandparent exists: select first or second child slot branchlessly via cmovne */
    lea rdx, [rax + NODE_FIRST]
    lea rcx, [rax + NODE_SECOND]
    cmp [rdx], r10
    cmovne rdx, rcx
    mov rcx, rdx

td_store_sib:
    mov [rcx], r11              /* Relink promoted sibling */

td_done:
    /* Push both detached leaf (r8) and split parent (r10) onto O(1) free-list! */
    mov rax, [free_node_head]
    mov [r10 + NODE_FIRST], rax
    mov [r8 + NODE_FIRST], r10
    mov [free_node_head], r8
    ret

/*
 * alloc_node() -> rax = pointer to new 32-byte aligned node
 * O(1) L1-hot Free-List recycler with fallback to bump allocator in node_pool.
 */
alloc_node:
    mov rax, [free_node_head]
    test rax, rax
    jz alloc_from_pool
    mov rdx, [rax + NODE_FIRST]
    mov [free_node_head], rdx
    prefetcht0 [rdx]            /* L1 Cache Prefetch: warm up next free node */
    jmp alloc_zero_node

alloc_from_pool:
    mov eax, [node_idx]
    cmp eax, 127                /* Support up to 128 nodes (~64 simultaneous windows) */
    jge alloc_fail
    inc dword ptr [node_idx]
    shl rax, 5                  /* rax * 32 bytes */
    lea rdx, [node_pool]
    add rax, rdx
alloc_zero_node:
    /* SIMD 128-bit Vectorized Node Zeroing (clears full 32 bytes in 2 stores) */
    pxor xmm0, xmm0
    movdqa [rax + 0], xmm0      /* Clears NODE_WIN, NODE_INFO, pad, NODE_FIRST (16 bytes) */
    movdqa [rax + 16], xmm0     /* Clears NODE_SECOND, NODE_PARENT (16 bytes) */
    ret
alloc_fail:
    lea rax, [node_pool]
    ret

bsp_first_leaf:
    test rdi, rdi
    jz bfl_null
bfl_loop:
    cmp dword ptr [rdi + NODE_WIN], 0
    jne bfl_found
    mov rdi, [rdi + NODE_FIRST]
    test rdi, rdi
    jnz bfl_loop
bfl_null:
    xor eax, eax
    ret
bfl_found:
    mov rax, rdi
    ret

/*
 * win_cache_put(edi = win_id, rsi = node_ptr)
 * Atomically registers window in 1-cycle MRU cache and 32-entry Direct Hash Table.
 * Clobbers: edx, rcx. Preserves all other registers.
 */
win_cache_put:
    test edi, edi
    jz 1f
    mov [win_mru_id], edi
    mov [win_mru_node], rsi
    mov edx, edi
    shr edx, 4
    xor edx, edi
    and edx, 31
    shl edx, 4
    lea rcx, [win_hash_tab + rdx]
    mov [rcx + 0], edi
    mov [rcx + 8], rsi
1:  ret

/*
 * win_cache_evict(edi = win_id)
 * Atomically invalidates window from MRU and Hash Table.
 * Clobbers: edx, rcx. Preserves all other registers.
 */
win_cache_evict:
    test edi, edi
    jz 1f
    cmp edi, [win_mru_id]
    jne 2f
    mov dword ptr [win_mru_id], 0
    mov qword ptr [win_mru_node], 0
2:
    mov edx, edi
    shr edx, 4
    xor edx, edi
    and edx, 31
    shl edx, 4
    lea rcx, [win_hash_tab + rdx]
    cmp [rcx + 0], edi
    jne 1f
    mov dword ptr [rcx + 0], 0
    mov qword ptr [rcx + 8], 0
1:  ret

/*
 * bsp_find_win_global(esi = win_id) -> rax = leaf_node, ecx = ws_idx (or rax=0 if not found)
 * Ultra-fast O(1) global window lookup across all workspaces.
 */
bsp_find_win_global:
    test esi, esi
    jz bfwg_null

    /* 1. Fast Path: 1-entry MRU Cache (~1 cycle) */
    cmp esi, [win_mru_id]
    jne bfwg_check_hash
    mov rax, [win_mru_node]
    test rax, rax
    jz bfwg_check_hash
    cmp [rax + NODE_WIN], esi
    jne bfwg_mru_inv
    movzx ecx, byte ptr [rax + NODE_INFO]
    shr ecx, 1
    and ecx, 0xF
    cmp ecx, WORKSPACE_COUNT
    jge bfwg_mru_inv
    ret

bfwg_mru_inv:
    mov dword ptr [win_mru_id], 0
    mov qword ptr [win_mru_node], 0

    /* 2. O(1) Path: 32-entry Direct Hash Table (~3 cycles) */
bfwg_check_hash:
    mov eax, esi
    shr eax, 4
    xor eax, esi
    and eax, 31
    shl eax, 4
    lea rdx, [win_hash_tab + rax]
    cmp [rdx + 0], esi
    jne bfwg_scan_fallback
    mov rax, [rdx + 8]
    test rax, rax
    jz bfwg_scan_fallback
    cmp [rax + NODE_WIN], esi
    jne bfwg_hash_inv
    movzx ecx, byte ptr [rax + NODE_INFO]
    shr ecx, 1
    and ecx, 0xF
    cmp ecx, WORKSPACE_COUNT
    jge bfwg_hash_inv
    /* HIT: Promote to MRU */
    mov [win_mru_id], esi
    mov [win_mru_node], rax
    ret

bfwg_hash_inv:
    mov dword ptr [rdx + 0], 0
    mov qword ptr [rdx + 8], 0

    /* 3. Fallback: Flat L1 linear scan of node_pool (zero stack, zero recursion) */
bfwg_scan_fallback:
    mov ecx, [node_idx]         /* total nodes allocated so far (<= 128) */
    test ecx, ecx
    jz bfwg_null
    lea rdx, [node_pool]

bfwg_scan_loop:
    cmp [rdx + NODE_WIN], esi
    je bfwg_found
    add rdx, 32                 /* 32 bytes per node */
    dec ecx
    jnz bfwg_scan_loop

bfwg_not_found:
bfwg_null:
    xor eax, eax
    ret

bfwg_found:
    mov edi, esi
    mov rsi, rdx                /* rsi = leaf node pointer */
    call win_cache_put
    mov rax, rsi
    /* Extract workspace index (0..8) into ecx */
    movzx ecx, byte ptr [rax + NODE_INFO]
    shr ecx, 1
    and ecx, 0xF
    ret

/*
 * bsp_update_history(rdi = focused_leaf_node)
 * Ascends from leaf to root, recording which child branch was active in bit 7 of parent's NODE_INFO:
 * bit 7 = 0 if FIRST child was active, bit 7 = 1 (0x80) if SECOND child was active.
 */
bsp_update_history:
    test rdi, rdi
    jz buh_ret
buh_loop:
    mov rax, [rdi + NODE_PARENT]
    test rax, rax
    jz buh_ret
    /* Branchless bit-setting: bit 7 = (child != FIRST) << 7 */
    cmp [rax + NODE_FIRST], rdi
    setne cl
    shl cl, 7
    and byte ptr [rax + NODE_INFO], 0x7F    /* Clear bit 7 */
    or byte ptr [rax + NODE_INFO], cl       /* Set bit 7 branchlessly */
    mov rdi, rax
    jmp buh_loop
buh_ret:
    ret

/*
 * bsp_arrange_ws(edi = ws_idx)
 * Standalone entry point: arranges workspace into [r15].
 */
bsp_arrange_ws:
    jmp bsp_arrange_ws_buf

.align 16
geom_init_offsets:
    .long GAP_PX
    .long GAP_PX + BAR_HEIGHT + GAP_PX
    .long -(GAP_PX * 2)
    .long -(BAR_HEIGHT + GAP_PX * 3)

/*
 * bsp_arrange_ws_buf(edi = ws_idx, r15 = write cursor)
 * Appends ConfigureWindow (32 bytes) packets for all windows in ws_idx to [r15].
 * Frameless Zero-Stack tail dispatch into bsp_arrange_node.
 */
bsp_arrange_ws_buf:
    lea rax, [bsp_roots]
    mov r8, [rax + 8*rdi]       /* r8 = root node */
    test r8, r8
    jz arrange_ws_done

    /* Get monitor geometry for this workspace (rdi = ws_idx) */
    call get_mon_for_ws         /* eax = 0 or 1 */
    shl eax, 4                  /* * 16 bytes */
    lea rcx, [mon_geom]
    add rcx, rax

    /* Vectorized root rect setup: loads [x, y, w, h] and applies gaps in 1 SIMD op */
    movdqu xmm0, [rcx]
    paddd xmm0, [geom_init_offsets]

    mov rdi, r8                 /* rdi = root node */
    jmp bsp_arrange_node        /* Pure Tail Call! */

arrange_ws_done:
    ret

/*
 * bsp_arrange_node(rdi = node, xmm0 = [x, y, w, h])
 * Writes 32-byte ConfigureWindow packets to [r15] cursor.
 * Vectorized layout engine: all 4 spatial dimensions [x, y, w, h] packed in 128-bit XMM.
 */
bsp_arrange_node:
    cmp dword ptr [rdi + NODE_WIN], 0
    jz do_ws_split

    /* Leaf window: direct write-combining 64-bit store burst (32 bytes ConfigureWindow) */
    mov eax, [rdi + NODE_WIN]
    shl rax, 32
    or rax, (8 << 16) | 12                   /* Opcode 12, len = 8 words */
    mov [r15 + 0], rax                       /* 64-bit store: header + win_id */
    mov dword ptr [r15 + 8], 31              /* value_mask: CWX|CWY|CWWidth|CWHeight|CWBorderWidth */
    movups [r15 + 12], xmm0                  /* SIMD 128-bit store: [x, y, w, h] written all at once! */
    mov dword ptr [r15 + 28], 1              /* border_width = 1 */
    add r15, 32
    ret

do_ws_split:
    push rdi                                 /* Save parent node (8 bytes) */
    sub rsp, 16                              /* Allocate 16 bytes for Child 2 rect */

    movq rax, xmm0                           /* rax = (y << 32) | x */
    movhlps xmm1, xmm0
    movq rdx, xmm1                           /* rdx = (h << 32) | w */

    mov r8d, eax                             /* r8d = x */
    mov r9, rax
    shr r9, 32                               /* r9d = y */
    mov r10d, edx                            /* r10d = w */
    mov r11, rdx
    shr r11, 32                              /* r11d = h */

    test byte ptr [rdi + NODE_INFO], 1       /* Bit 0 = split_type (0=V, 1=H) */
    jnz ws_split_horizontal

    /* Vertical Split (X axis): r10d = w, r8d = x */
    mov ecx, r10d
    shr ecx, 1                               /* half_w = w / 2 */
    lea esi, [rcx + 2]                       /* delta = (w / 2) + 2 */
    sub ecx, 2                               /* w1 = half_w - 2 */
    sub r10d, esi                            /* w2 = w - delta */
    add r8d, esi                             /* x2 = x + delta */

    /* Child 1: low = rax ([x, y]), high = (h << 32) | w1 */
    mov rdx, r11
    shl rdx, 32
    mov ecx, ecx
    or rdx, rcx                              /* rdx = (h << 32) | w1 */

    /* Child 2: low = (y << 32) | x2, high = (h << 32) | w2 */
    mov rcx, r9
    shl rcx, 32
    mov r8d, r8d
    or rcx, r8                               /* rcx = (y << 32) | x2 */
    mov rsi, r11
    shl rsi, 32
    mov r10d, r10d
    or rsi, r10                              /* rsi = (h << 32) | w2 */
    jmp ws_split_dispatch

ws_split_horizontal:
    /* Horizontal Split (Y axis): r11d = h, r9d = y */
    mov ecx, r11d
    shr ecx, 1                               /* half_h = h / 2 */
    lea esi, [rcx + 2]                       /* delta = (h / 2) + 2 */
    sub ecx, 2                               /* h1 = half_h - 2 */
    sub r11d, esi                            /* h2 = h - delta */
    add r9d, esi                             /* y2 = y + delta */

    /* Child 1: low = rax ([x, y]), high = (h1 << 32) | w */
    mov rdx, rcx
    shl rdx, 32
    mov r10d, r10d
    or rdx, r10                              /* rdx = (h1 << 32) | w */

    /* Child 2: low = (y2 << 32) | x, high = (h2 << 32) | w */
    mov rcx, r9
    shl rcx, 32
    mov r8d, r8d
    or rcx, r8                               /* rcx = (y2 << 32) | x */
    mov rsi, r11
    shl rsi, 32
    mov r10d, r10d
    or rsi, r10                              /* rsi = (h2 << 32) | w */

ws_split_dispatch:
    /* Store Child 2 to [rsp] (16 bytes, naturally 64-bit aligned!) */
    mov [rsp + 0], rcx                       /* Child 2 [x2, y2] */
    mov [rsp + 8], rsi                       /* Child 2 [w2, h2] */

    /* Pack Child 1 into xmm0 directly in registers */
    movq xmm0, rax                           /* Child 1 [x, y] */
    movq xmm1, rdx                           /* Child 1 [w, h] */
    punpcklqdq xmm0, xmm1                    /* xmm0 = Child 1 [x, y, w, h] */

    /* Dispatch Child 1 */
    mov rdi, [rsp + 16]                      /* Parent node */
    mov rdi, [rdi + NODE_FIRST]
    call bsp_arrange_node

    /* Dispatch Child 2 (zero recomputation!) */
    mov rdi, [rsp + 16]                      /* Parent node */
    mov rdi, [rdi + NODE_SECOND]
    movups xmm0, [rsp]                       /* Child 2 rect */
    add rsp, 24                              /* Clean up frame + parent (16 + 8) */
    jmp bsp_arrange_node                     /* Pure Tail Call! */


/*
 * bsp_node_in_direction(rdi = start_leaf, esi = orient, edx = dir)
 * True 2D BSP Tree Directional Traversal (Zero-Stack Frameless)
 * orient: 0 = Vertical (h/l), 1 = Horizontal (k/j)
 * dir:    0 = Left / Up (first), 1 = Right / Down (second)
 * Scratch registers used: rdi, esi, edx, r8, r9, rax
 */
bsp_node_in_direction:
    test rdi, rdi
    jz nid_null
    cmp dword ptr [rdi + NODE_WIN], 0
    je nid_null

nid_climb:
    mov r8, [rdi + NODE_PARENT]
    test r8, r8
    jz nid_null                 /* Reached root without finding matching split */

    /* Check parent split orientation (bit 0 of NODE_INFO) */
    movzx eax, byte ptr [r8 + NODE_INFO]
    and eax, 1
    cmp eax, esi
    jne nid_climb_next

    /* Branchless check: on_first == (dir != 0) -> crosses boundary */
    cmp [r8 + NODE_FIRST], rdi
    sete al
    cmp al, dl
    je nid_found_branch

nid_climb_next:
    mov rdi, r8
    jmp nid_climb

nid_found_branch:
    /* Branchless child selection: edx == 0 ? first : second */
    mov r9, [r8 + NODE_SECOND]
    mov rax, [r8 + NODE_FIRST]
    test edx, edx
    cmovz r9, rax

dive_down:
    test r9, r9
    jz nid_null
    cmp dword ptr [r9 + NODE_WIN], 0
    jne nid_success             /* Found destination leaf! */

    movzx eax, byte ptr [r9 + NODE_INFO]
    and eax, 1
    cmp eax, esi
    je dive_same_orient

    /* Perpendicular split: follow last-focused history bit (bit 7 of NODE_INFO) branchlessly */
    test byte ptr [r9 + NODE_INFO], 0x80
    mov rax, [r9 + NODE_SECOND]
    mov r9, [r9 + NODE_FIRST]
    cmovnz r9, rax
    jmp dive_down

dive_same_orient:
    /* Same orientation: take opposite side of dir to land on closest neighbor branchlessly */
    mov rax, [r9 + NODE_FIRST]
    mov r9, [r9 + NODE_SECOND]
    test edx, edx
    cmovnz r9, rax
    jmp dive_down

nid_success:
    mov rax, r9
    ret

nid_null:
    xor eax, eax
    ret

exit_clean:
    call kill_xorg
    push SYS_EXIT
    pop rax
    xor edi, edi
    syscall

exit_fail:
    call kill_xorg
    push SYS_EXIT
    pop rax
    push 1
    pop rdi
    syscall

/* kill_xorg: if we spawned Xorg, SIGTERM it and reap it */
kill_xorg:
    mov rdi, [xorg_pid]
    test rdi, rdi
    jle 1f
    push 62                     /* SYS_KILL */
    pop rax
    push 15                     /* SIGTERM */
    pop rsi
    syscall
    mov rdi, [xorg_pid]
    push SYS_WAIT4
    pop rax
    xor esi, esi
    xor edx, edx
    xor r10d, r10d
    syscall
    mov qword ptr [xorg_pid], 0
1:  ret

/*
 * start_xorg: fork + exec "/usr/bin/X :0 -ac [vtN] -keeptty".
 * vtN is derived from stdin's tty (/proc/self/fd/0 -> /dev/ttyN); the child
 * inherits stdin so Xorg.wrap sees a console user.
 * Returns child pid in rax (<= 0 on failure).
 */
start_xorg:
    push 89                     /* SYS_READLINK */
    pop rax
    lea rdi, [proc_fd0]
    lea rsi, [tty_buf]
    push 31
    pop rdx
    syscall
    cmp rax, 9                  /* need at least "/dev/ttyN" */
    jl 2f
    mov byte ptr [tty_buf + rax], 0
    mov ecx, [tty_buf + 5]
    and ecx, 0xffffff
    cmp ecx, 0x797474           /* "tty" */
    jne 2f
    lea rsi, [tty_buf + 8]
    lea rdi, [vt_arg + 2]
3:  mov al, [rsi]
    cmp al, '0'
    jb 4f
    cmp al, '9'
    ja 4f
    mov [rdi], al
    inc rsi
    inc rdi
    jmp 3b
4:  cmp rdi, offset vt_arg + 2
    je 2f                       /* no digits: tty is not /dev/ttyN */
    lea rax, [vt_arg]
    mov [x_argv + 24], rax      /* ... -ac vtN -keeptty */
    lea rax, [x_keep_str]
    mov [x_argv + 32], rax
2:  push SYS_FORK
    pop rax
    syscall
    test rax, rax
    jnz 5f
    lea rdi, [x_path]
    lea rsi, [x_argv]
    mov rdx, [saved_envp]
    mov al, SYS_EXECVE          /* rax == 0 in child! */
    syscall
    push SYS_EXIT
    pop rax
    push 1
    pop rdi
    syscall
5:  ret

.data
sock_addr:
    .short AF_UNIX
sock_path:
    .ascii "/tmp/.X11-unix/X"
sock_path_num:
    .ascii "0\0\0\0"
    .space 90, 0

x11_setup_req:
    .byte 'l', 0, 11, 0
    .byte 0, 0, 0, 0
    .byte 0, 0, 0, 0

disp_env_str:
    .asciz "DISPLAY=:0"
x_path:
    .asciz "/usr/bin/X"
x_disp:
    .asciz ":0"
x_ac:
    .asciz "-ac"
x_keep_str:
    .asciz "-keeptty"
proc_fd0:
    .asciz "/proc/self/fd/0"
vt_arg:
    .asciz "vt\0\0\0\0\0\0\0\0\0\0\0\0"
.align 8
x_argv:
    .quad x_path, x_disp, x_ac, x_keep_str, 0, 0

sh_bin:
    .asciz "/bin/sh"
sh_arg_c:
    .asciz "-c"

cmd_menu:
    .asciz "[ -x $HOME/dotfiles/scripts/nviciod.sh ] && $HOME/dotfiles/scripts/nviciod.sh || ([ -x $HOME/dotfile/dmenuscript ] && $HOME/dotfile/dmenuscript || st)"

cmd_term:
    .asciz "[ \"$DISPLAY\" != \":0\" ] && st || (ftasm 2>/dev/null || st)"
    /* .asciz "st || alacritty || kitty" */

cmd_autostart:
    .asciz "command -v setxkbmap >/dev/null 2>&1 && setxkbmap -option caps:escape; command -v xset >/dev/null 2>&1 && xset r rate 180 250; command -v xset >/dev/null 2>&1 && xset +fp /home/a/.fonts && xset fp rehash"

.align 8
sh_argv_menu:
    .quad sh_bin
    .quad sh_arg_c
    .quad cmd_menu
    .quad 0

sh_argv_term:
    .quad sh_bin
    .quad sh_arg_c
    .quad cmd_term
    .quad 0

sh_argv_autostart:
    .quad sh_bin
    .quad sh_arg_c
    .quad cmd_autostart
    .quad 0

/*
 * Bit-Packed Workspace Bar & Monitor Table (1 byte per workspace = 9 bytes total!):
 * - bit 0:    monitor index (0 = Monitor 0, 1 = Monitor 1) -> read with `and ecx, 1`
 * - bits 1-7: precomputed even X pixel coordinate (10 + slot * 24) -> read with `and esi, 0xFE`
 */
.align 8
ws_mon_table:
    .byte (10  | 1)             /* WS 1 -> Monitor 1, X = 10 */
    .byte (34  | 1)             /* WS 2 -> Monitor 1, X = 34 */
    .byte (58  | 1)             /* WS 3 -> Monitor 1, X = 58 */
    .byte (82  | 1)             /* WS 4 -> Monitor 1, X = 82 */
    .byte (106 | 1)             /* WS 5 -> Monitor 1, X = 106 */
    .byte (130 | 1)             /* WS 6 -> Monitor 1, X = 130 */
    .byte (154 | 1)             /* WS 7 -> Monitor 1, X = 154 */
    .byte (178 | 1)             /* WS 8 -> Monitor 1, X = 178 */
    .byte (10  | 0)             /* WS 9 -> Monitor 0, X = 10 */

/*
 * Table of Grabbed Shortcuts:
 * [Keycode, Modifiers]
 */
shortcut_keys:
    /* Basic WM shortcuts */
    .byte 38, 0x08              /* Alt + a (Workspace Prev) */
    .byte 39, 0x08              /* Alt + s (Workspace Next) */
    .byte 40, 0x08              /* Alt + d */
    .byte 25, 0x08              /* Alt + w */
    .byte 36, 0x08              /* Alt + Return */
    .byte 24, 0x08              /* Alt + q */
    .byte 24, 0x09              /* Alt + Shift + q */
    .byte 41, 0x08              /* Alt + f */
    .byte 41, 0x09              /* Alt + Shift + f (Floating Toggle) */
    .byte 65, 0x08              /* Alt + Space (Floating Toggle) */
    .byte 43, 0x08              /* Alt + h (Focus Left) */
    .byte 44, 0x08              /* Alt + j (Focus Down) */
    .byte 45, 0x08              /* Alt + k (Focus Up) */
    .byte 46, 0x08              /* Alt + l (Focus Right) */
    .byte 43, 0x09              /* Alt + Shift + h (Move Left) */
    .byte 44, 0x09              /* Alt + Shift + j (Move Down) */
    .byte 45, 0x09              /* Alt + Shift + k (Move Up) */
    .byte 46, 0x09              /* Alt + Shift + l (Move Right) */
    .byte 55, 0x08              /* Alt + v */
    .byte 56, 0x08              /* Alt + b */

    /* Workspaces 1..9: Alt + 1..9 (keycodes 10 to 18) */
    .byte 10, 0x08
    .byte 11, 0x08
    .byte 12, 0x08
    .byte 13, 0x08
    .byte 14, 0x08
    .byte 15, 0x08
    .byte 16, 0x08
    .byte 17, 0x08
    .byte 18, 0x08

    /* Move window to WS: Alt + Shift + 1..9 */
    .byte 10, 0x09
    .byte 11, 0x09
    .byte 12, 0x09
    .byte 13, 0x09
    .byte 14, 0x09
    .byte 15, 0x09
    .byte 16, 0x09
    .byte 17, 0x09
    .byte 18, 0x09

    .byte 0, 0                  /* Sentinel */

.bss
.align 8
/* sock_fd lives permanently in register r13d (Zero RAM!) */
root_win:       .long 0
screen_w:       .long 0
screen_h:       .long 0
bar_win:        .long 0, 0      /* bar_win for Monitor 0 and Monitor 1 */
bar_gc:         .long 0         /* base GC ID (bar_gc+0=bg, +1=active, +2=inactive) */
atom_wm_proto:  .long 0         /* WM_PROTOCOLS atom */
atom_wm_del:    .long 0         /* WM_DELETE_WINDOW atom */
saved_envp:     .quad 0
xorg_pid:       .quad 0
/*
 * cur_ws lives permanently in register r14 (Zero RAM!).
 * All 4 state flags live permanently in 8-bit register bpl (Zero RAM!).
 */

/* Monitor configuration: up to 2 monitors (16 bytes each: x, y, w, h) */
.align 16
mon_geom:       .space 32
mon_cur_ws:     .byte 0, 0      /* Currently visible workspace index per monitor */
                .space 6        /* Pad to 40 bytes */

.align 64
/* 9 Workspaces state (Cache-line aligned) */
bsp_roots:      .space 72       /* 9 x 8 bytes pointers */
.align 64
bsp_focuses:    .space 72       /* 9 x 8 bytes pointers */
last_focused_win: .long 0       /* Last window with active border (for de-focus) */
free_node_head: .quad 0         /* O(1) recycled BSP node free-list head */

.align 64
win_mru_id:     .long 0
                .long 0
win_mru_node:   .quad 0
win_hash_tab:   .space 512      /* 32 entries * 16 bytes (win_id, pad, node_ptr) */

node_idx:       .long 0
ev_q_head:      .long 0         /* Current read offset in event_queue */
ev_q_tail:      .long 0         /* Total bytes available in event_queue */

/* Drag State for Mod + Mouse Move/Resize */
drag_win:       .long 0         /* Target window being moved or resized */
drag_btn:       .long 0         /* 1 = Move (Button 1), 3 = Resize (Button 3) */
drag_off_x:     .long 0         /* Relative click X offset inside window */
drag_off_y:     .long 0         /* Relative click Y offset inside window */
drag_orig_w:    .long 0         /* Window W at drag start */
drag_orig_h:    .long 0         /* Window H at drag start */
drag_start_x:   .long 0         /* Mouse Root X at drag start */
drag_start_y:   .long 0         /* Mouse Root Y at drag start */
drag_last_coords: .quad 0       /* Last rendered (Y<<32)|X or (H<<32)|W for Zero-Delta filtering */

/*
 * Shared I/O Buffer (1056 bytes total) + node_pool (512 bytes) = 1568 bytes contiguous!
 * Entire ELF binary (.text + .data + ALL .bss) fits inside 2 physical 4 KB pages [0x400000..0x401fff]!
 */
.align 64
env_buf:        .space 160      /* Up to 19 env vars + DISPLAY=:0 + NULL for TTY login */
tty_buf:        .space 32

.align 64
out_buf:
in_buf:         .space 2048
event_queue:    .space 2048     /* Buffer for up to 64 X11 events (32 bytes each) */

/* 128 nodes of 32 bytes = 4096 bytes (64-byte aligned, pairs fit in exact L1 cache lines!) */
.align 64
node_pool:      .space 4096
