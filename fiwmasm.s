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
.equ BORDER_ACTIVE,   0xFFFFFF
.equ BORDER_INACTIVE, 0x2D2D2D

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

event_loop:
    /* Check if event_queue still has buffered events */
    mov eax, [ev_q_head]
    cmp eax, [ev_q_tail]
    jl ev_pop_buffered

    /* Buffer is empty, read next chunk of events (up to 9 events = 288 bytes) */
    xor eax, eax                /* SYS_READ = 0 (2 bytes!) */
    mov edi, r13d
    lea rsi, [event_queue]
    mov edx, 288
    syscall

    test eax, eax
    jle exit_clean

    /* Initialize queue head = 0, tail = bytes read */
    mov dword ptr [ev_q_head], 0
    mov [ev_q_tail], eax

ev_pop_buffered:
    /* Zero-Copy Event Dispatch: read directly from [event_queue + rcx] in rsi! */
    mov ecx, [ev_q_head]
    lea rsi, [event_queue + rcx]
    add dword ptr [ev_q_head], 32

    movzx eax, byte ptr [rsi]
    and al, 0x7F

    cmp al, 2                   /* KeyPress */
    je handle_key

    cmp al, 4                   /* ButtonPress */
    je handle_btn_press

    cmp al, 5                   /* ButtonRelease */
    je handle_btn_release

    cmp al, 6                   /* MotionNotify */
    je handle_motion

    cmp al, 7                   /* EnterNotify */
    je handle_enter

    cmp al, 12                  /* Expose */
    je handle_expose

    cmp al, 20                  /* MapRequest */
    je handle_map

    sub al, 17                  /* 17 (DestroyNotify) -> 0, 18 (UnmapNotify) -> 1 */
    cmp al, 1
    jbe handle_unmap

    jmp event_loop

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

    /* win_x = rootX - eventX, win_y = rootY - eventY */
    mov edi, ecx
    sub edi, r9d                        /* win_x */
    mov esi, r8d
    sub esi, r10d                       /* win_y */

    /* Save drag state */
    mov [drag_win], edx
    mov [drag_btn], eax
    mov [drag_start_x], ecx
    mov [drag_start_y], r8d
    mov [drag_orig_x], edi
    mov [drag_orig_y], esi

    /* Find window in active workspace BSP tree */
    lea rcx, [bsp_roots]
    mov rdi, [rcx + 8*r14]
    push rdx
    mov esi, edx
    call bsp_find_node_by_win
    pop rdx
    test rax, rax
    jz event_loop                       /* Not a managed window */

    /* Update focus to clicked window */
    mov rbx, rax
    lea rax, [bsp_focuses]
    mov [rax + 8*r14], rbx
    mov rdi, rbx
    call bsp_update_history
    mov edi, [drag_win]
    call set_focus

    /* Query current window width and height via X_GetGeometry (Opcode 14, 8 bytes) */
    mov edx, [drag_win]
    mov dword ptr [out_buf + 0], (2 << 16) | 14
    mov [out_buf + 4], edx

    push SYS_WRITE
    pop rax
    mov edi, r13d
    lea rsi, [out_buf]
    push 8
    pop rdx
    syscall

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

    /* Raise window (stack mode = Above) */
    mov edx, [drag_win]
    mov dword ptr [out_buf + 0], (4 << 16) | 12
    mov [out_buf + 4], edx
    mov dword ptr [out_buf + 8], 64     /* CWStackMode = Above */
    mov dword ptr [out_buf + 12], 0
    push SYS_WRITE
    pop rax
    mov edi, r13d
    lea rsi, [out_buf]
    push 16
    pop rdx
    syscall

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

    movsx ecx, word ptr [rsi + 20]      /* current rootX */
    movsx r8d, word ptr [rsi + 22]      /* current rootY */

    cmp dword ptr [drag_btn], 3         /* Button 3 = Resize */
    je motion_resize

    /* Button 1 = Move */
    /* new_x = orig_x + (cur_x - start_x) */
    mov eax, ecx
    sub eax, [drag_start_x]
    add eax, [drag_orig_x]              /* new_x */

    /* new_y = orig_y + (cur_y - start_y) */
    mov esi, r8d
    sub esi, [drag_start_y]
    add esi, [drag_orig_y]              /* new_y */

    /* ConfigureWindow (x, y) */
    mov dword ptr [out_buf + 0], (5 << 16) | 12
    mov [out_buf + 4], edx
    mov dword ptr [out_buf + 8], 3      /* CWX | CWY = 3 */
    mov [out_buf + 12], eax
    mov [out_buf + 16], esi

    push SYS_WRITE
    pop rax
    mov edi, r13d
    lea rsi, [out_buf]
    push 20
    pop rdx
    syscall
    jmp event_loop

motion_resize:
    /* new_w = MAX(50, orig_w + (cur_x - start_x)) */
    mov eax, ecx
    sub eax, [drag_start_x]
    add eax, [drag_orig_w]
    cmp eax, 50
    jge resize_w_ok
    mov eax, 50
resize_w_ok:

    /* new_h = MAX(50, orig_h + (cur_y - start_y)) */
    mov esi, r8d
    sub esi, [drag_start_y]
    add esi, [drag_orig_h]
    cmp esi, 50
    jge resize_h_ok
    mov esi, 50
resize_h_ok:

    /* ConfigureWindow (w, h) */
    mov dword ptr [out_buf + 0], (5 << 16) | 12
    mov [out_buf + 4], edx
    mov dword ptr [out_buf + 8], 12     /* CWWidth | CWHeight = 12 */
    mov [out_buf + 12], eax
    mov [out_buf + 16], esi

    push SYS_WRITE
    pop rax
    mov edi, r13d
    lea rsi, [out_buf]
    push 20
    pop rdx
    syscall
    jmp event_loop

handle_btn_release:
    mov dword ptr [drag_win], 0
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

    /* Find leaf node with this window in active workspace */
    lea rax, [bsp_roots]
    mov rdi, [rax + 8*r14]
    push rdx
    mov esi, edx
    call bsp_find_node_by_win
    pop rdx
    test rax, rax
    jz event_loop               /* Window not in active workspace BSP tree */

    /* Check if already focused */
    lea rbx, [bsp_focuses]
    cmp rax, [rbx + 8*r14]
    je event_loop               /* Already focused */

    /* Update focus */
    mov [rbx + 8*r14], rax
    mov rdi, rax
    push rdx
    call bsp_update_history
    pop rdx
    mov edi, edx
    call set_focus
    jmp event_loop

handle_expose:
    mov edx, [rsi + 4]          /* drawable/window */
    cmp edx, [bar_win + 0]
    je check_expose_count
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

    call draw_all_bars
    jmp event_loop

/*
 * =========================================================================
 * KEY EVENT HANDLERS
 * =========================================================================
 */
handle_key:
    movzx ecx, byte ptr [rsi + 1]   /* keycode */
    movzx edx, word ptr [rsi + 28]  /* state/modifiers */

    /* Check Alt+1..9 (keycodes 10 to 18) */
    cmp ecx, 10
    jl check_other_keys
    cmp ecx, 18
    jg check_other_keys

    sub ecx, 10                 /* ecx = target workspace 0..8 */
    test edx, 1                 /* ShiftMask = 1 ? */
    jnz key_move_to_ws
    jmp key_focus_ws

key_focus_ws:
    mov edi, ecx
    call focus_workspace
    jmp event_loop

key_move_to_ws:
    mov edi, ecx
    call move_window_to_workspace
    jmp event_loop

check_other_keys:
    /* Alt+a (38): Workspace Anterior (Prev) */
    cmp ecx, 38
    je key_ws_prev

    /* Alt+s (39): Próxima Workspace (Next) */
    cmp ecx, 39
    je key_ws_next

    /* Alt+d (40): Menu / scripts */
    cmp ecx, 40
    je key_menu

    /* Alt+w (25) or Alt+Return (36): Terminal */
    cmp ecx, 25
    je key_term
    cmp ecx, 36
    je key_term

    /* Alt+q (24) */
    cmp ecx, 24
    jne check_nav_keys
    test edx, 1                 /* ShiftMask = 1 */
    jnz exit_clean              /* Alt+Shift+q: Quit WM */
    jmp key_kill                /* Alt+q: Kill Active Window */

check_nav_keys:
    /* Alt+Shift+f or Alt+Space (65): Toggle Floating */
    cmp ecx, 65
    je key_floating

    /* Alt+f (41) */
    cmp ecx, 41
    jne check_hjkl
    test edx, 1                 /* Shift pressed? */
    jnz key_floating            /* Alt+Shift+f: Floating Toggle */
    jmp key_fullscreen          /* Alt+f: Fullscreen Toggle */

check_hjkl:
    /* Navigation with HJKL (keycodes 43, 44, 45, 46) */
    cmp ecx, 43
    jl check_split_keys
    cmp ecx, 46
    jg check_split_keys

    test edx, 1                 /* Shift pressed? */
    jnz key_move_hjkl           /* Alt + Shift + HJKL: Swap/Move window between splits */
    jmp key_focus_hjkl          /* Alt + HJKL: Change focus between splits */

check_split_keys:
    /* Alt+v (55): Select Next Split = Horizontal for focused window */
    cmp ecx, 55
    je key_split_h

    /* Alt+b (56): Select Next Split = Vertical for focused window */
    cmp ecx, 56
    je key_split_v

    jmp event_loop

key_focus_hjkl:
    /* ecx has keycode: 43=h (left), 44=j (down), 45=k (up), 46=l (right) */
    lea rbx, [bsp_focuses]
    mov rdi, [rbx + 8*r14]      /* start leaf */
    test rdi, rdi
    jz event_loop

    /* Determine orient (esi) and dir (edx) from keycode in ecx */
    call hjkl_decode_dir
    call bsp_node_in_direction
    test rax, rax
    jz event_loop               /* No window in that direction */

    /* Set new focus */
    lea rbx, [bsp_focuses]
    mov [rbx + 8*r14], rax
    mov rdi, rax
    call bsp_update_history
    lea rbx, [bsp_focuses]
    mov rax, [rbx + 8*r14]
    mov edi, [rax + NODE_WIN]
    call set_focus
    jmp event_loop

key_move_hjkl:
    /* Swap focused window with neighbor in the specified 2D direction */
    lea rbx, [bsp_focuses]
    mov r12, [rbx + 8*r14]      /* r12 = current focused leaf */
    test r12, r12
    jz event_loop

    mov rdi, r12
    call hjkl_decode_dir
    call bsp_node_in_direction
    test rax, rax
    jz event_loop               /* No neighbor in that direction to swap */

    /* Swap window IDs between r12 and rax */
    mov r8d, [r12 + NODE_WIN]
    mov r9d, [rax + NODE_WIN]
    mov [r12 + NODE_WIN], r9d
    mov [rax + NODE_WIN], r8d

    /* Update focus to follow the moved window */
    lea rbx, [bsp_focuses]
    mov [rbx + 8*r14], rax
    mov rdi, rax
    call bsp_update_history
    lea rbx, [bsp_focuses]
    mov rax, [rbx + 8*r14]

    /* Re-arrange grid and set focus in ONE atomic syscall */
    push r8
    lea r15, [out_buf]
    mov edi, r14d
    call bsp_arrange_ws_buf
    pop rdi
    call set_focus_buf
    call flush_out_buf
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
    mov eax, r14d
    dec eax
    test eax, eax
    jns ws_prev_ok
    mov eax, WORKSPACE_COUNT - 1   /* Volta para o 8 (workspace 9) */
ws_prev_ok:
    mov edi, eax
    call focus_workspace
    jmp event_loop

key_ws_next:
    mov eax, r14d
    inc eax
    cmp eax, WORKSPACE_COUNT
    jl ws_next_ok
    xor eax, eax                   /* Volta para o 0 (workspace 1) */
ws_next_ok:
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
     * Terminate client window and process tree (52 bytes batched in 1 syscall):
     * 1. X_SendEvent (Opcode 25, len=11 words = 44 bytes): sends ICCCM ClientMessage
     *    WM_PROTOCOLS / WM_DELETE_WINDOW so apps like Brave/Chrome cleanly shut down
     *    all background audio and renderer processes.
     * 2. X_KillClient (Opcode 113, len=2 words = 8 bytes): forces the X server to
     *    sever the client's socket connection so non-ICCCM apps die immediately too.
     */
    mov dword ptr [out_buf + 0],  (11 << 16) | 25   /* X_SendEvent: propagate=0, len=11 */
    mov [out_buf + 4],  edx                         /* destination = win */
    mov dword ptr [out_buf + 8],  0                 /* event_mask = NoEventMask */
    mov dword ptr [out_buf + 12], (32 << 8) | 33    /* ClientMessage (33), format=32 */
    mov [out_buf + 16], edx                         /* window = win */
    mov eax, [atom_wm_proto]
    mov [out_buf + 20], eax                         /* message_type = WM_PROTOCOLS */
    mov eax, [atom_wm_del]
    mov [out_buf + 24], eax                         /* data.l[0] = WM_DELETE_WINDOW */
    mov qword ptr [out_buf + 28], 0                 /* data.l[1..2] = 0 */
    mov qword ptr [out_buf + 36], 0                 /* data.l[3..4] = 0 */

    /* Also append X_KillClient (8 bytes) */
    mov dword ptr [out_buf + 44], (2 << 16) | 113
    mov [out_buf + 48], edx

    push SYS_WRITE
    pop rax
    mov edi, r13d
    lea rsi, [out_buf]
    push 52
    pop rdx
    syscall
    jmp event_loop

key_fullscreen:
    lea rbx, [bsp_focuses]
    mov rax, [rbx + 8*r14]
    test rax, rax
    jz event_loop
    mov edx, [rax + NODE_WIN]
    test edx, edx
    jz event_loop

    xor bpl, 1                  /* Toggle bit 0 (is_fullscreen) */
    test bpl, 1
    jz fs_restore

    /* Set fullscreen on target monitor */
    mov edi, r14d
    call get_mon_for_ws         /* eax = monitor index */
    mov ecx, eax
    shl ecx, 4                  /* 16 bytes per monitor */
    lea rbx, [mon_geom]
    add rbx, rcx

    mov dword ptr [out_buf + 0], (8 << 16) | 12
    mov [out_buf + 4], edx
    mov dword ptr [out_buf + 8], 31             /* x, y, w, h, border_width */
    mov eax, [rbx + 0]          /* x */
    mov [out_buf + 12], eax
    mov eax, [rbx + 4]          /* y */
    mov [out_buf + 16], eax
    mov eax, [rbx + 8]          /* w */
    mov [out_buf + 20], eax
    mov eax, [rbx + 12]         /* h */
    mov [out_buf + 24], eax
    mov dword ptr [out_buf + 28], 0             /* border_width = 0 for fullscreen */

    /* X_RaiseWindow (ConfigureWindow, stack mode = Above) so the
       fullscreen window actually covers splits from other columns */
    mov dword ptr [out_buf + 32], (4 << 16) | 12
    mov [out_buf + 36], edx
    mov dword ptr [out_buf + 40], 64
    mov dword ptr [out_buf + 44], 0

    push SYS_WRITE
    pop rax
    mov edi, r13d
    lea rsi, [out_buf]
    push 48
    pop rdx
    syscall
    jmp event_loop

fs_restore:
    mov edi, r14d
    call bsp_arrange_ws
    jmp event_loop

key_floating:
    lea rbx, [bsp_focuses]
    mov rax, [rbx + 8*r14]
    test rax, rax
    jz event_loop
    mov edx, [rax + NODE_WIN]
    test edx, edx
    jz event_loop

    xor bpl, 32                 /* Toggle bit 5 (is_floating) */
    test bpl, 32
    jz fl_restore

    /* Set floating: 60% dimensions centered on target monitor */
    mov edi, r14d
    call get_mon_for_ws         /* eax = monitor index */
    mov ecx, eax
    shl ecx, 4                  /* 16 bytes per monitor */
    lea rbx, [mon_geom]
    add rbx, rcx

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

    /* Packet 1: ConfigureWindow (x, y, w, h, border_width = 1) (32 bytes) */
    mov dword ptr [out_buf + 0], (8 << 16) | 12
    mov [out_buf + 4], edx
    mov dword ptr [out_buf + 8], 31             /* x, y, w, h, border_width */
    mov [out_buf + 12], esi                     /* x */
    mov [out_buf + 16], edi                     /* y */
    mov [out_buf + 20], r8d                     /* w */
    mov [out_buf + 24], r9d                     /* h */
    mov dword ptr [out_buf + 28], 1             /* border_width = 1 */

    /* Packet 2: X_RaiseWindow (ConfigureWindow, stack mode = Above) (16 bytes) */
    mov dword ptr [out_buf + 32], (4 << 16) | 12
    mov [out_buf + 36], edx
    mov dword ptr [out_buf + 40], 64            /* CWStackMode = 64 */
    mov dword ptr [out_buf + 44], 0             /* Above = 0 */

    push SYS_WRITE
    pop rax
    mov edi, r13d
    lea rsi, [out_buf]
    push 48
    pop rdx
    syscall
    jmp event_loop

fl_restore:
    mov edi, r14d
    call bsp_arrange_ws
    jmp event_loop

set_focus:
    lea r15, [out_buf]
    call set_focus_buf
    jmp flush_out_buf

/*
 * set_focus_buf(edi = win_id, r15 = write cursor)
 * 1. If edi == 0 (no window focused / empty workspace):
 *    - Restores border_inactive on last_focused_win if one was focused.
 *    - Resets last_focused_win = 0.
 *    - Sets input focus to root_win (PointerRoot = 1).
 * 2. If edi != 0:
 *    - Restores border_inactive (0x2D2D2D) on last_focused_win if different.
 *    - Sets border_active (0xFFFFFF) on new win_id.
 *    - Appends 12-byte X_SetInputFocus request to [r15].
 */
set_focus_buf:
    test edi, edi
    jnz sfb_has_win

    /* Clear focus: de-focus old window and give focus to root */
    mov eax, [last_focused_win]
    test eax, eax
    jz sfb_root_focus
    mov dword ptr [r15 + 0], (4 << 16) | 2   /* Opcode 2: ChangeWindowAttributes */
    mov [r15 + 4], eax
    mov dword ptr [r15 + 8], 8               /* CWBorderPixel */
    mov dword ptr [r15 + 12], BORDER_INACTIVE
    add r15, 16
    mov dword ptr [last_focused_win], 0

sfb_root_focus:
    mov dword ptr [r15 + 0], (3 << 16) | (1 << 8) | 42  /* Opcode 42: SetInputFocus, revert_to = 1 (PointerRoot) */
    mov eax, [root_win]
    mov [r15 + 4], eax
    mov dword ptr [r15 + 8], 0
    add r15, 12
    ret

sfb_has_win:
    mov eax, [last_focused_win]
    cmp eax, edi
    je sfb_do_input_focus       /* Already has active border */

    /* If old window exists, paint border_inactive (0x2D2D2D) */
    test eax, eax
    jz sfb_paint_active
    mov dword ptr [r15 + 0], (4 << 16) | 2   /* Opcode 2: ChangeWindowAttributes */
    mov [r15 + 4], eax
    mov dword ptr [r15 + 8], 8               /* CWBorderPixel */
    mov dword ptr [r15 + 12], BORDER_INACTIVE
    add r15, 16

sfb_paint_active:
    /* Paint new window border_active (0xFFFFFF) */
    mov dword ptr [r15 + 0], (4 << 16) | 2   /* Opcode 2: ChangeWindowAttributes */
    mov [r15 + 4], edi
    mov dword ptr [r15 + 8], 8               /* CWBorderPixel */
    mov dword ptr [r15 + 12], BORDER_ACTIVE
    add r15, 16

    mov [last_focused_win], edi

sfb_do_input_focus:
    mov dword ptr [r15 + 0], (3 << 16) | (1 << 8) | 42
    mov [r15 + 4], edi
    mov dword ptr [r15 + 8], 0
    add r15, 12
focus_ret:
    ret

/*
 * flush_out_buf(r15 = write cursor at end of out_buf)
 * Sends all bytes accumulated in [out_buf .. r15) in ONE single sys_write!
 */
flush_out_buf:
    lea rsi, [out_buf]
    mov rdx, r15
    sub rdx, rsi
    jle flush_ret
    push SYS_WRITE
    pop rax
    mov edi, r13d
    syscall
flush_ret:
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
    push rbx
    push r12
    push r15

    mov r12d, edi               /* r12d = new_ws */
    cmp r12d, r14d
    je fw_done

    lea r15, [out_buf]          /* r15 = unified batch write cursor */

    /* Determine target monitor m (0 or 1) */
    mov edi, r12d
    call get_mon_for_ws         /* eax = m */
    mov edx, eax                /* edx = target monitor m */

    /* Get old workspace currently shown on target monitor m */
    movzx ebx, byte ptr [mon_cur_ws + rdx]  /* ebx = old_ws on monitor m */
    cmp ebx, r12d
    je fw_same_ws

    /* Update visible workspace on target monitor */
    mov [mon_cur_ws + rdx], r12b

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

    mov eax, edx
    xor eax, 1                  /* other monitor = m ^ 1 */
    movzx eax, byte ptr [mon_cur_ws + rax]
    cmp eax, ebx
    je fw_bar_and_focus         /* Other monitor is viewing old_ws: do NOT hide! */

fw_do_hide:
    push rdx
    lea rax, [bsp_roots]
    mov rdi, [rax + 8*rbx]
    call hide_tree_collect
    pop rdx
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
    test rdi, rdi
    jz fw_focus_empty
    push rdi
    call bsp_update_history
    pop rdi
    mov edi, [rdi + NODE_WIN]
    call set_focus_buf
    jmp fw_flush

fw_focus_empty:
    xor edi, edi
    call set_focus_buf
    /* X_ClearArea on root window (Opcode 61, exposures=1, w=0, h=0 clears whole root) (16 bytes) */
    mov dword ptr [r15 + 0], (4 << 16) | (1 << 8) | 61
    mov eax, [root_win]
    mov [r15 + 4], eax
    mov qword ptr [r15 + 8], 0
    add r15, 16

fw_flush:
    call flush_out_buf

fw_done:
    pop r15
    pop r12
    pop rbx
    ret

/*
 * move_window_to_workspace(edi = target_ws 0..8)
 * Batches hiding the moved window + rearranging the current workspace into 1 syscall!
 */
move_window_to_workspace:
    push rbx
    push r12
    push r15

    mov r12d, edi               /* target_ws */
    cmp r12d, r14d
    je mw_done

    /* Get current focused leaf */
    lea rbx, [bsp_focuses]
    mov rdi, [rbx + 8*r14]
    test rdi, rdi
    jz mw_done

    mov ebx, [rdi + NODE_WIN]   /* ebx = win */
    test ebx, ebx
    jz mw_done

    /* Detach from current workspace */
    push rdi
    call bsp_tree_detach
    pop rdi

    /* Batch hide window + re-arrange current workspace in ONE syscall */
    lea r15, [out_buf]
    mov dword ptr [r15 + 0], (5 << 16) | 12  /* Opcode 12: ConfigureWindow */
    mov [r15 + 4], ebx                       /* window ID */
    mov dword ptr [r15 + 8], 3               /* CWX | CWY = 3 */
    mov dword ptr [r15 + 12], -9999          /* x = -9999 (off-screen) */
    mov dword ptr [r15 + 16], -9999          /* y = -9999 (off-screen) */
    add r15, 20

    mov edi, r14d
    call bsp_arrange_ws_buf
    call flush_out_buf

    /* Insert window into target workspace tree */
    mov edi, ebx
    mov esi, r12d
    call bsp_tree_insert_into_ws

mw_done:
    pop r15
    pop r12
    pop rbx
    ret

/*
 * hide_tree_collect(rdi = node, r15 = write cursor):
 * Appends 20-byte ConfigureWindow (x=-9999, y=-9999) for all leaf windows in tree to [r15].
 */
hide_tree_collect:
    test rdi, rdi
    jz htc_ret
    cmp dword ptr [rdi + NODE_WIN], 0
    je htc_recurse

    /* Leaf window found: append 20-byte ConfigureWindow packet to [r15] */
    mov eax, [rdi + NODE_WIN]
    shl rax, 32
    or rax, (5 << 16) | 12                   /* Opcode 12: ConfigureWindow (len=5) */
    mov [r15 + 0], rax                       /* [0..3]=header, [4..7]=win_id */
    mov rax, -9999
    shl rax, 32
    or rax, 3                                /* [8..11]=value_mask (3), [12..15]=x (-9999) */
    mov [r15 + 8], rax
    mov dword ptr [r15 + 16], -9999          /* y = -9999 (off-screen) */
    add r15, 20
    ret

htc_recurse:
    push rdi
    mov rdi, [rdi + NODE_FIRST]
    call hide_tree_collect
    pop rdi
    mov rdi, [rdi + NODE_SECOND]
    jmp hide_tree_collect
htc_ret:
    ret

/*
 * =========================================================================
 * TOP STATUS BAR RENDERING (Pure X11 Wire Protocol)
 * Supports Independent Bars per Monitor!
 * =========================================================================
 */
init_bar:
    push rbx
    push r12
    push r15

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
    mov dword ptr [r15 + 32], 0x181818          /* background pixel */
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
       - bar_gc + 0: active workspace digit GC (fg = 0xc2c2b0, bg = 0x181818)
       - bar_gc + 1: inactive workspace digit GC (fg = 0x685742, bg = 0x181818)
     */
    mov ecx, [bar_gc]
    mov edx, [bar_win + 0]

    /* GC 0: Active text + background cell (24 bytes) */
    mov dword ptr [r15 + 0], (6 << 16) | 55
    mov [r15 + 4], ecx
    mov [r15 + 8], edx
    mov dword ptr [r15 + 12], 12                /* GCForeground (4) | GCBackground (8) */
    mov dword ptr [r15 + 16], 0xc2c2b0          /* fg = active cream/gold */
    mov dword ptr [r15 + 20], 0x181818          /* bg = bar dark background */

    /* GC 1: Inactive text + background cell (24 bytes) */
    inc ecx
    mov dword ptr [r15 + 24], (6 << 16) | 55
    mov [r15 + 28], ecx
    mov [r15 + 32], edx
    mov dword ptr [r15 + 36], 12
    mov dword ptr [r15 + 40], 0x685742          /* fg = inactive brown/gray */
    mov dword ptr [r15 + 44], 0x181818          /* bg = bar dark background */

    /* 4. SelectInput on root window (16 bytes) */
    mov dword ptr [r15 + 48], (4 << 16) | 2
    mov eax, [root_win]
    mov [r15 + 52], eax
    mov dword ptr [r15 + 56], 0x00000800        /* CWEventMask bit 11 */
    mov dword ptr [r15 + 60], 0x00180001        /* SubstructureRedirect | SubstructureNotify | KeyPress */
    add r15, 64

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

    pop r15
    pop r12
    pop rbx
    ret

/*
 * draw_all_bars():
 * Standalone entry point for Expose events: renders bars into out_buf and flushes.
 */
draw_all_bars:
    push r15
    lea r15, [out_buf]
    call draw_all_bars_buf
    call flush_out_buf
    pop r15
    ret

/*
 * draw_all_bars_buf(r15 = write cursor):
 * Full 9-digit bar redraw (used on initial boot and Expose events).
 */
draw_all_bars_buf:
    push rbx
    xor ebx, ebx                                /* ebx = ws_idx (0..8) */
dab_ws_loop:
    mov edi, ebx
    call draw_single_ws_buf
    inc ebx
    cmp ebx, WORKSPACE_COUNT
    jl dab_ws_loop
    pop rbx
    ret

/*
 * draw_single_ws_buf(edi = ws_idx 0..8, r15 = write cursor):
 * Surgical O(1) Branchless Single-Digit Bar Renderer via X_ImageText8 (Opcode 76, 20 bytes):
 * Bit-slices both monitor (bit 0) and X offset (bits 1..7) from a single packed byte!
 */
draw_single_ws_buf:
    push rbx
    mov ebx, edi                                /* ebx = target ws_idx (0..8) */

    xor ecx, ecx                                /* default single-monitor: mon_idx = 0 */
    imul esi, ebx, 24
    add esi, 10                                 /* default single-monitor X = 10 + ws_idx * 24 */

    test bpl, 8                                 /* bit 3 = is_dual_mon */
    jz dsw_emit

    movzx ecx, byte ptr [ws_mon_table + rbx]    /* 1-byte packed load: (X & ~1) | mon_idx */
    mov esi, ecx
    and ecx, 1                                  /* bit 0 = mon_idx (0 or 1) */
    and esi, 0xFE                               /* bits 1..7 = precomputed even X coordinate! */

dsw_emit:
    /* Select pre-created GC: bar_gc + 0 (active) or bar_gc + 1 (inactive) */
    mov edx, [bar_gc]
    cmp ebx, r14d
    je dsw_gc_ok
    inc edx                                     /* inactive: bar_gc + 1 */
dsw_gc_ok:
    /* X_ImageText8 (Opcode 76, n=1 char, length=5 words = 20 bytes) */
    mov dword ptr [r15 + 0], (5 << 16) | (1 << 8) | 76
    mov eax, [bar_win + 4*rcx]
    mov [r15 + 4], eax
    mov [r15 + 8], edx

    /* Pack X (low 16b) and Y=13 (high 16b) */
    or esi, (13 << 16)
    mov [r15 + 12], esi

    /* Character byte '1' + ebx */
    lea eax, [rbx + '1']
    mov [r15 + 16], eax

    add r15, 20
    pop rbx
    ret

/*
 * =========================================================================
 * DUAL MONITOR DETECTION VIA XINERAMA EXTENSION
 * =========================================================================
 */
detect_monitors:
    push rbx

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

    pop rbx
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

    /* Insert into active workspace BSP tree */
    mov edi, r12d
    mov esi, r14d
    call bsp_tree_insert_into_ws

    /* Zero-Flicker Map Pipeline:
       1. ConfigureWindow (bsp_arrange_ws_buf) FIRST while new window is still unmapped!
       2. ChangeWindowAttributes (Select EnterWindowMask = 0x10) so focus-follows-mouse works!
       3. MapWindow SECOND so window appears directly at its final tiled geometry!
       4. SetInputFocus THIRD, all flushed in ONE atomic sys_write! */
    lea r15, [out_buf]
    mov edi, r14d
    call bsp_arrange_ws_buf

    /* ChangeWindowAttributes: CWEventMask (bit 11 = 0x800) = EnterWindowMask (bit 4 = 0x10) (16 bytes) */
    mov dword ptr [r15 + 0], (4 << 16) | 2
    mov [r15 + 4], r12d
    mov dword ptr [r15 + 8], 0x800
    mov dword ptr [r15 + 12], 0x10
    add r15, 16

    /*
     * X_GrabButton on client window (Opcode 28, length 6 words = 24 bytes):
     * Grab Button 1 (Left Click, Move) and Button 3 (Right Click, Resize) with Mod1 (0x08).
     * Also grabs combo with NumLock (0x18) so mouse shortcuts always trigger!
     */
    /* Button 1 + Mod1 (0x08) */
    mov dword ptr [r15 + 0], (6 << 16) | 28
    mov [r15 + 4], r12d
    mov dword ptr [r15 + 8], (1 << 24) | (1 << 16) | 0x4C
    mov dword ptr [r15 + 12], 0
    mov dword ptr [r15 + 16], 0
    mov dword ptr [r15 + 20], (0x08 << 16) | 1

    /* Button 1 + Mod1 | NumLock (0x18) */
    mov dword ptr [r15 + 24], (6 << 16) | 28
    mov [r15 + 28], r12d
    mov dword ptr [r15 + 32], (1 << 24) | (1 << 16) | 0x4C
    mov dword ptr [r15 + 36], 0
    mov dword ptr [r15 + 40], 0
    mov dword ptr [r15 + 44], (0x18 << 16) | 1

    /* Button 3 + Mod1 (0x08) */
    mov dword ptr [r15 + 48], (6 << 16) | 28
    mov [r15 + 52], r12d
    mov dword ptr [r15 + 56], (1 << 24) | (1 << 16) | 0x4C
    mov dword ptr [r15 + 60], 0
    mov dword ptr [r15 + 64], 0
    mov dword ptr [r15 + 68], (0x08 << 16) | 3

    /* Button 3 + Mod1 | NumLock (0x18) */
    mov dword ptr [r15 + 72], (6 << 16) | 28
    mov [r15 + 76], r12d
    mov dword ptr [r15 + 80], (1 << 24) | (1 << 16) | 0x4C
    mov dword ptr [r15 + 84], 0
    mov dword ptr [r15 + 88], 0
    mov dword ptr [r15 + 92], (0x18 << 16) | 3
    add r15, 96

    mov dword ptr [r15 + 0], (2 << 16) | 8
    mov dword ptr [r15 + 4], r12d
    add r15, 8

    mov edi, r12d
    call set_focus_buf
    call flush_out_buf
    jmp event_loop

handle_unmap:
    mov edx, [rsi + 8]
    test edx, edx
    jz event_loop

    /* Find leaf node with this window across all workspaces */
    xor ecx, ecx
find_ws_unmap:
    lea rax, [bsp_roots]
    mov rdi, [rax + 8*rcx]
    push rcx
    push rdx
    mov esi, edx
    call bsp_find_node_by_win
    pop rdx
    pop rcx
    test rax, rax
    jnz found_unmap_node
    inc ecx
    cmp ecx, WORKSPACE_COUNT
    jl find_ws_unmap
    jmp event_loop

found_unmap_node:
    /* Detach node from its workspace */
    mov rdi, rax
    call bsp_tree_detach

    /* If it was in active workspace, re-arrange and focus in ONE syscall */
    cmp ecx, r14d
    jne event_loop

    lea r15, [out_buf]
    lea rax, [bsp_roots]
    mov rdi, [rax + 8*rcx]
    call bsp_first_leaf
    lea rbx, [bsp_focuses]
    mov [rbx + 8*rcx], rax
    test rax, rax
    jz no_focus_left
    push rax
    mov rdi, rax
    call bsp_update_history
    pop rax
    mov edi, [rax + NODE_WIN]
    call set_focus_buf
    jmp unmap_arrange_and_flush
no_focus_left:
    xor edi, edi
    call set_focus_buf
    /* X_ClearArea on root window (Opcode 61, exposures=1, w=0, h=0 clears whole root) (16 bytes) */
    mov dword ptr [r15 + 0], (4 << 16) | (1 << 8) | 61
    mov eax, [root_win]
    mov [r15 + 4], eax
    mov qword ptr [r15 + 8], 0
    add r15, 16
unmap_arrange_and_flush:
    mov edi, r14d
    call bsp_arrange_ws_buf
    call flush_out_buf
    jmp event_loop

/*
 * bsp_tree_insert_into_ws(edi = win, esi = ws_idx)
 */
bsp_tree_insert_into_ws:
    push rbx
    push r12
    push r13
    push r14

    mov r14d, esi               /* ws_idx */

    call alloc_node
    mov [rax + NODE_WIN], edi
    lea edx, [r14 + r14]        /* edx = ws_idx << 1 */
    mov [rax + NODE_INFO], dl   /* packed NODE_INFO: (ws_idx << 1) | 0 */
    mov r12, rax                /* r12 = new leaf */

    lea rax, [bsp_roots]
    mov rbx, [rax + 8*r14]
    test rbx, rbx
    jnz ws_split_leaf

    /* First window on this workspace becomes root */
    lea rax, [bsp_roots]
    mov [rax + 8*r14], r12
    lea rax, [bsp_focuses]
    mov [rax + 8*r14], r12
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

ws_split_leaf:
    lea rax, [bsp_focuses]
    mov rbx, [rax + 8*r14]
    test rbx, rbx
    jnz ws_has_focus
    lea rax, [bsp_roots]
    mov rdi, [rax + 8*r14]
    call bsp_first_leaf
    mov rbx, rax
ws_has_focus:
    call alloc_node
    mov r13, rax                /* r13 = new split node */

    /* Determine split direction from bpl bits 1..2: 1 = V, 2 = H, 0 = auto */
    mov ecx, ebp
    shr cl, 1
    and cl, 3
    jnz ws_use_chosen

    /* Alternate split: invert bit 0 of parent's NODE_INFO */
    mov rax, [rbx + NODE_PARENT]
    test rax, rax
    jz ws_def_v
    mov cl, [rax + NODE_INFO]
    and cl, 1
    xor cl, 1
    jmp ws_apply_split
ws_def_v:
    xor cl, cl
    jmp ws_apply_split

ws_use_chosen:
    dec cl                      /* 1 -> 0 (V), 2 -> 1 (H) */
    and bpl, ~6                 /* clear next_split_sel bits 1..2 */

ws_apply_split:
    lea edx, [r14 + r14]        /* ws_idx << 1 */
    or cl, dl                   /* pack (ws_idx << 1) | split_type into 1 byte! */
    mov [r13 + NODE_INFO], cl
    mov rax, [rbx + NODE_PARENT]
    mov [r13 + NODE_PARENT], rax

    mov [r13 + NODE_FIRST], rbx
    mov [r13 + NODE_SECOND], r12
    mov [rbx + NODE_PARENT], r13
    mov [r12 + NODE_PARENT], r13

    mov rax, [r13 + NODE_PARENT]
    test rax, rax
    jz ws_new_root

    cmp [rax + NODE_FIRST], rbx
    jne ws_link_second
    mov [rax + NODE_FIRST], r13
    jmp ws_insert_done
ws_link_second:
    mov [rax + NODE_SECOND], r13
    jmp ws_insert_done

ws_new_root:
    lea rax, [bsp_roots]
    mov [rax + 8*r14], r13

ws_insert_done:
    lea rax, [bsp_focuses]
    mov [rax + 8*r14], r12
    mov rdi, r12
    call bsp_update_history
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

/*
 * bsp_tree_detach(rdi = leaf_node)
 */
bsp_tree_detach:
    push rbx
    push r12
    push r13
    push r14

    mov rbx, rdi
    movzx r14d, byte ptr [rbx + NODE_INFO]
    shr r14d, 1                             /* Extract ws_idx = NODE_INFO >> 1 */
    mov r12, [rbx + NODE_PARENT]
    test r12, r12
    jnz td_has_parent

    /* Removing root: push leaf node (rbx) onto O(1) free-list! */
    mov rax, [free_node_head]
    mov [rbx + NODE_FIRST], rax
    mov [free_node_head], rbx

    lea rax, [bsp_roots]
    mov qword ptr [rax + 8*r14], 0
    lea rax, [bsp_focuses]
    mov qword ptr [rax + 8*r14], 0
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

td_has_parent:
    mov r13, [r12 + NODE_FIRST]
    cmp r13, rbx
    jne td_sib_first
    mov r13, [r12 + NODE_SECOND]
td_sib_first:

    mov rax, [r12 + NODE_PARENT]
    mov [r13 + NODE_PARENT], rax

    test rax, rax
    jz td_sib_root

    cmp [rax + NODE_FIRST], r12
    jne td_link_gp_sec
    mov [rax + NODE_FIRST], r13
    jmp td_done
td_link_gp_sec:
    mov [rax + NODE_SECOND], r13
    jmp td_done

td_sib_root:
    lea rax, [bsp_roots]
    mov [rax + 8*r14], r13

td_done:
    /* Push both detached leaf (rbx) and split parent (r12) onto O(1) free-list! */
    mov rax, [free_node_head]
    mov [r12 + NODE_FIRST], rax
    mov [rbx + NODE_FIRST], r12
    mov [free_node_head], rbx

    pop r14
    pop r13
    pop r12
    pop rbx
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
    mov qword ptr [rax + 0],  0  /* Clears NODE_WIN, NODE_SPLIT, NODE_WS, pad */
    mov qword ptr [rax + 8],  0  /* Clears NODE_FIRST */
    mov qword ptr [rax + 16], 0  /* Clears NODE_SECOND */
    mov qword ptr [rax + 24], 0  /* Clears NODE_PARENT */
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

bsp_find_node_by_win:
    test rdi, rdi
    jz bfn_null
    cmp [rdi + NODE_WIN], esi
    je bfn_found
    cmp dword ptr [rdi + NODE_WIN], 0
    jne bfn_null

    push rdi
    mov rdi, [rdi + NODE_FIRST]
    call bsp_find_node_by_win
    pop rdi
    test rax, rax
    jnz bfn_ret

    mov rdi, [rdi + NODE_SECOND]
    jmp bsp_find_node_by_win

bfn_found:
    mov rax, rdi
    ret
bfn_null:
    xor eax, eax
bfn_ret:
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
    cmp [rax + NODE_FIRST], rdi
    jne buh_second
    and byte ptr [rax + NODE_INFO], ~0x80   /* Clear bit 7: FIRST was focused */
    mov rdi, rax
    jmp buh_loop
buh_second:
    or byte ptr [rax + NODE_INFO], 0x80    /* Set bit 7: SECOND was focused */
    mov rdi, rax
    jmp buh_loop
buh_ret:
    ret

/*
 * bsp_arrange_ws(edi = ws_idx)
 * Standalone entry point: arranges workspace into out_buf and flushes.
 */
bsp_arrange_ws:
    push r15
    lea r15, [out_buf]
    call bsp_arrange_ws_buf
    call flush_out_buf
    pop r15
    ret

/*
 * bsp_arrange_ws_buf(edi = ws_idx, r15 = write cursor)
 * Appends ConfigureWindow (28 bytes) packets for all windows in ws_idx to [r15].
 */
bsp_arrange_ws_buf:
    push rbx
    push r12
    push r13
    push r14

    mov r12d, edi               /* ws_idx */

    lea rax, [bsp_roots]
    mov r13, [rax + 8*r12]      /* r13 = root node */
    test r13, r13
    jz arrange_ws_done

    /* Get monitor geometry for this workspace */
    mov edi, r12d
    call get_mon_for_ws         /* eax = 0 or 1 */
    shl eax, 4                  /* * 16 bytes */
    lea rbx, [mon_geom]
    add rbx, rax

    mov rdi, r13                /* rdi = root node */
    mov esi, [rbx + 0]
    add esi, GAP_PX             /* x */
    mov edx, [rbx + 4]
    add edx, GAP_PX + BAR_HEIGHT + GAP_PX  /* y (leave room for top bar!) */
    mov r8d, [rbx + 8]
    sub r8d, GAP_PX * 2         /* w */
    mov r9d, [rbx + 12]
    sub r9d, BAR_HEIGHT + GAP_PX * 3       /* h (leave room for top bar!) */

    call bsp_arrange_node

arrange_ws_done:
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

/*
 * bsp_arrange_node(rdi=node, esi=x, edx=y, r8=w, r9=h)
 * Writes 28-byte ConfigureWindow packets to [r15] cursor.
 * Register-only recursion (only 40 bytes pushed on split nodes, 0 stack frame on leaf nodes!):
 */
bsp_arrange_node:
    cmp dword ptr [rdi + NODE_WIN], 0
    jz do_ws_split

    /* Leaf window: zero stack allocation! Configure geometry + border_width=1 (32 bytes) */
    mov eax, [rdi + NODE_WIN]
    shl rax, 32
    or rax, (8 << 16) | 12                   /* Opcode 12: ConfigureWindow (8 dwords = 32 bytes) */
    mov [r15 + 0], rax                       /* header + win_id in 1 store! */
    mov eax, esi                             /* x */
    shl rax, 32
    or rax, 31                               /* CWX | CWY | CWWidth | CWHeight | CWBorderWidth */
    mov [r15 + 8], rax                       /* value_mask + x in 1 store! */
    mov eax, r8d                             /* w */
    shl rax, 32
    mov edx, edx
    or rax, rdx                              /* y | (w << 32) */
    mov [r15 + 16], rax                      /* y + w in 1 store! */
    mov rax, 1                               /* border_width = 1 */
    shl rax, 32
    mov r9d, r9d
    or rax, r9                               /* h | (1 << 32) */
    mov [r15 + 24], rax                      /* h + border_width in 1 store! */
    add r15, 32
    ret

do_ws_split:
    push rdi
    push rsi
    push rdx
    push r8
    push r9

    test byte ptr [rdi + NODE_INFO], 1      /* Bit 0 = split_type (0=V, 1=H) */
    jnz ws_split_horizontal

    /* Vertical Split */
    shr r8d, 1
    sub r8d, GAP_PX / 2         /* w1 */
    mov rdi, [rdi + NODE_FIRST]
    call bsp_arrange_node

    mov rdi, [rsp + 32]
    mov rdi, [rdi + NODE_SECOND]
    mov esi, [rsp + 24]
    mov eax, [rsp + 8]
    shr eax, 1
    add esi, eax
    add esi, GAP_PX - (GAP_PX / 2)  /* x2 = x + w1 + gap */
    mov edx, [rsp + 16]
    mov r8d, [rsp + 8]
    sub r8d, esi
    add r8d, [rsp + 24]         /* w2 = w - (x2 - x) */
    mov r9d, [rsp + 0]
    add rsp, 40
    jmp bsp_arrange_node

ws_split_horizontal:
    /* Horizontal Split */
    shr r9d, 1
    sub r9d, GAP_PX / 2         /* h1 */
    mov rdi, [rdi + NODE_FIRST]
    call bsp_arrange_node

    mov rdi, [rsp + 32]
    mov rdi, [rdi + NODE_SECOND]
    mov esi, [rsp + 24]
    mov edx, [rsp + 16]
    mov eax, [rsp + 0]
    shr eax, 1
    add edx, eax
    add edx, GAP_PX - (GAP_PX / 2)  /* y2 = y + h1 + gap */
    mov r8d, [rsp + 8]
    mov r9d, [rsp + 0]
    sub r9d, edx
    add r9d, [rsp + 16]         /* h2 = h - (y2 - y) */
    add rsp, 40
    jmp bsp_arrange_node


/*
 * bsp_node_in_direction(rdi = start_leaf, esi = orient, edx = dir)
 * True 2D BSP Tree Directional Traversal
 * orient: 0 = Vertical (h/l), 1 = Horizontal (k/j)
 * dir:    0 = Left / Up (first), 1 = Right / Down (second)
 */
bsp_node_in_direction:
    push rbx
    push r12
    push r13
    push r14
    push r15

    test rdi, rdi
    jz nid_null
    cmp dword ptr [rdi + NODE_WIN], 0
    je nid_null

    mov r12, rdi                /* r12 = current node p */
    mov r13d, esi               /* r13d = orient */
    mov r14d, edx               /* r14d = dir */

nid_climb:
    mov rbx, [r12 + NODE_PARENT]
    test rbx, rbx
    jz nid_null                 /* Reached root without finding matching split */

    /* Check parent split orientation (bit 0 of NODE_INFO) */
    movzx eax, byte ptr [rbx + NODE_INFO]
    and eax, 1
    cmp eax, r13d
    jne nid_climb_next

    /* Check if current child is first or second */
    mov eax, 1                  /* on_first = 1 */
    cmp [rbx + NODE_FIRST], r12
    je on_first_done
    xor eax, eax                /* on_first = 0 */
on_first_done:

    /* want_first = (dir == 0) */
    mov edx, 1
    test r14d, r14d
    jz want_first_done
    xor edx, edx
want_first_done:

    /* if (on_first != want_first) we can cross this split boundary */
    cmp eax, edx
    jne nid_found_branch

nid_climb_next:
    mov r12, rbx
    jmp nid_climb

nid_found_branch:
    /* c = want_first ? parent->first : parent->second */
    test edx, edx
    jnz take_first_child
    mov r15, [rbx + NODE_SECOND]
    jmp dive_down
take_first_child:
    mov r15, [rbx + NODE_FIRST]

dive_down:
    test r15, r15
    jz nid_null
    cmp dword ptr [r15 + NODE_WIN], 0
    jne nid_success             /* Found destination leaf! */

    movzx eax, byte ptr [r15 + NODE_INFO]
    and eax, 1
    cmp eax, r13d
    je dive_same_orient

    /* Perpendicular split: follow last-focused history bit (bit 7 of NODE_INFO) */
    test byte ptr [r15 + NODE_INFO], 0x80
    jnz dive_perp_second
    mov r15, [r15 + NODE_FIRST]
    jmp dive_down
dive_perp_second:
    mov r15, [r15 + NODE_SECOND]
    jmp dive_down

dive_same_orient:
    /* Same orientation: take opposite side of dir to land on closest neighbor */
    test r14d, r14d
    jz dive_take_second
    mov r15, [r15 + NODE_FIRST]
    jmp dive_down
dive_take_second:
    mov r15, [r15 + NODE_SECOND]
    jmp dive_down

nid_success:
    mov rax, r15
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

nid_null:
    xor eax, eax
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
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
    .asciz "st"
    /* .asciz "st || alacritty || kitty" */

cmd_autostart:
    .asciz "command -v setxkbmap >/dev/null 2>&1 && setxkbmap -option caps:escape; command -v xset >/dev/null 2>&1 && xset r rate 180 250"

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
mon_geom:       .space 32
mon_cur_ws:     .byte 0, 0      /* Currently visible workspace index per monitor */


/* 9 Workspaces state */
bsp_roots:      .space 72       /* 9 x 8 bytes pointers */
bsp_focuses:    .space 72       /* 9 x 8 bytes pointers */
last_focused_win: .long 0       /* Last window with active border (for de-focus) */
free_node_head: .quad 0         /* O(1) recycled BSP node free-list head */

node_idx:       .long 0
ev_q_head:      .long 0         /* Current read offset in event_queue */
ev_q_tail:      .long 0         /* Total bytes available in event_queue */

/* Drag State for Mod + Mouse Move/Resize */
drag_win:       .long 0         /* Target window being moved or resized */
drag_btn:       .long 0         /* 1 = Move (Button 1), 3 = Resize (Button 3) */
drag_orig_x:    .long 0         /* Window X at drag start */
drag_orig_y:    .long 0         /* Window Y at drag start */
drag_orig_w:    .long 0         /* Window W at drag start */
drag_orig_h:    .long 0         /* Window H at drag start */
drag_start_x:   .long 0         /* Mouse Root X at drag start */
drag_start_y:   .long 0         /* Mouse Root Y at drag start */

/*
 * Shared I/O Buffer (1056 bytes total) + node_pool (512 bytes) = 1568 bytes contiguous!
 * Entire ELF binary (.text + .data + ALL .bss) fits inside 2 physical 4 KB pages [0x400000..0x401fff]!
 */
.align 8
env_buf:        .space 160      /* Up to 19 env vars + DISPLAY=:0 + NULL for TTY login */
tty_buf:        .space 32

.align 8
out_buf:
in_buf:         .space 2048
event_queue:    .space 288      /* Buffer for up to 9 X11 events (32 bytes each) */

/* 128 nodes of 32 bytes = 4096 bytes (allocated on demand, Zero disk ELF space!) */
.align 8
node_pool:      .space 4096
