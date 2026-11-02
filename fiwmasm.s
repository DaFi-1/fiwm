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

/*
 * Compact Node Structure (32 bytes aligned - 50% smaller!):
 * offset 0:  dword client_win (if leaf) OR 0 (if internal split node) [4 bytes]
 * offset 4:  byte  split_type (0 = vertical, 1 = horizontal)          [1 byte]
 * offset 5:  byte  ws_idx (0..8)                                      [1 byte]
 * offset 6:  word  pad                                                [2 bytes]
 * offset 8:  qword first_child (pointer)                              [8 bytes]
 * offset 16: qword second_child (pointer)                             [8 bytes]
 * offset 24: qword parent (pointer)                                   [8 bytes]
 */
.equ NODE_WIN,     0
.equ NODE_SPLIT,   4
.equ NODE_WS,      5
.equ NODE_FIRST,   8
.equ NODE_SECOND,  16
.equ NODE_PARENT,  24
.equ NODE_SIZE,    32

.text
_start:
    xor r14d, r14d              /* r14 = permanent zero-RAM state register (cur_ws=0) */
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
    mov byte ptr [disp_found], 1
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
    cmp byte ptr [disp_found], 0
    jne env_ok
    mov rsi, [saved_envp]
    lea rdi, [env_buf]
    mov ecx, 30
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
    /* 1. sys_socket(AF_UNIX, SOCK_STREAM, 0) */
    mov eax, SYS_SOCKET
    mov edi, AF_UNIX
    mov esi, SOCK_STREAM
    xor edx, edx
    syscall
    test eax, eax
    js exit_fail
    mov r13d, eax               /* r13d = permanent zero-RAM socket fd! */

    /* 2. sys_connect(sock_fd, &sock_addr, sizeof(sock_addr)) */
    mov eax, SYS_CONNECT
    mov edi, r13d
    lea rsi, [sock_addr]
    mov edx, 110
    syscall
    test eax, eax
    jns connected_ok

    /* Connect failed: close the socket */
    mov eax, SYS_CLOSE
    mov edi, r13d
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
    mov eax, SYS_POLL           /* poll(NULL, 0, 20): sleep 20ms */
    xor edi, edi
    xor esi, esi
    mov edx, 20
    syscall
    jmp do_connect

connected_ok:
    /* 3. Send X11 Setup Request (12 bytes) */
    mov eax, SYS_WRITE
    mov edi, r13d
    lea rsi, [x11_setup_req]
    mov edx, 12
    syscall

    /* 4. Read X11 Setup Reply Header (8 bytes) */
    mov eax, SYS_READ
    mov edi, r13d
    lea rsi, [in_buf]
    mov edx, 8
    syscall
    cmp byte ptr [in_buf], 1
    jne exit_fail

    movzx ecx, word ptr [in_buf + 6]
    shl ecx, 2                  /* ecx = remaining setup reply bytes to drain */

    /* Read first chunk into in_buf (up to 576 bytes - fits in 1 KB I/O overlay!) */
    mov eax, SYS_READ
    mov edi, r13d
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

    /* Drain all remaining setup reply bytes from socket */
drain_setup_loop:
    test ecx, ecx
    jle setup_drained
    mov edx, 576
    cmp edx, ecx
    cmovg edx, ecx
    push rcx
    mov eax, SYS_READ
    mov edi, r13d
    lea rsi, [in_buf]
    syscall
    pop rcx
    test eax, eax
    jle setup_drained
    sub ecx, eax
    jmp drain_setup_loop

setup_drained:
    /* Detect Dual Monitor (Xinerama) */
    call detect_monitors

    /* Create and map top status bar + SelectInput on root in ONE batched syscall */
    call init_bar

    /* 6. Grab Shortcut Keys (including Alt+1..9 and Alt+Shift+1..9) */
    call grab_all_keys

    /* 7. Autostart (i3 parity): keyboard layout + repeat rate.
       setxkbmap resets X auto-repeat, so xset must run after it. */
    lea rsi, [sh_argv_autostart]
    call spawn_cmd

event_loop:
    /* Check if event_queue still has buffered events */
    mov eax, [ev_q_head]
    cmp eax, [ev_q_tail]
    jl ev_pop_buffered

    /* Buffer is empty, read next chunk of events (up to 9 events = 288 bytes) */
    mov eax, SYS_READ
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

    cmp al, 12                  /* Expose */
    je handle_expose

    cmp al, 20                  /* MapRequest */
    je handle_map

    cmp al, 18                  /* UnmapNotify */
    je handle_unmap

    cmp al, 17                  /* DestroyNotify */
    je handle_unmap

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
    /* Alt+f (41): Toggle Fullscreen */
    cmp ecx, 41
    je key_fullscreen

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

    /* Determine orient and dir */
    cmp ecx, 43                 /* h = Left */
    jne not_focus_h
    xor esi, esi                /* orient = 0 (Vertical) */
    xor edx, edx                /* dir = 0 (Left) */
    jmp do_dir_focus
not_focus_h:
    cmp ecx, 46                 /* l = Right */
    jne not_focus_l
    xor esi, esi                /* orient = 0 (Vertical) */
    mov edx, 1                  /* dir = 1 (Right) */
    jmp do_dir_focus
not_focus_l:
    cmp ecx, 45                 /* k = Up */
    jne not_focus_k
    mov esi, 1                  /* orient = 1 (Horizontal) */
    xor edx, edx                /* dir = 0 (Up) */
    jmp do_dir_focus
not_focus_k:
    /* j = Down */
    mov esi, 1                  /* orient = 1 (Horizontal) */
    mov edx, 1                  /* dir = 1 (Down) */

do_dir_focus:
    call bsp_node_in_direction
    test rax, rax
    jz event_loop               /* No window in that direction */

    /* Set new focus */
    lea rbx, [bsp_focuses]
    mov [rbx + 8*r14], rax
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
    cmp ecx, 43                 /* h = Left */
    jne not_move_h
    xor esi, esi                /* Vertical */
    xor edx, edx                /* Left */
    jmp do_dir_move
not_move_h:
    cmp ecx, 46                 /* l = Right */
    jne not_move_l
    xor esi, esi                /* Vertical */
    mov edx, 1                  /* Right */
    jmp do_dir_move
not_move_l:
    cmp ecx, 45                 /* k = Up */
    jne not_move_k
    mov esi, 1                  /* Horizontal */
    xor edx, edx                /* Up */
    jmp do_dir_move
not_move_k:
    /* j = Down */
    mov esi, 1                  /* Horizontal */
    mov edx, 1                  /* Down */

do_dir_move:
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

    /* Re-arrange grid and set focus in ONE atomic syscall */
    push r8
    lea r15, [out_buf]
    mov edi, r14d
    call bsp_arrange_ws_buf
    pop rdi
    call set_focus_buf
    call flush_out_buf
    jmp event_loop

key_split_h:
    mov byte ptr [next_split_sel], 2   /* 2 = horizontal */
    jmp event_loop

key_split_v:
    mov byte ptr [next_split_sel], 1   /* 1 = vertical */
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

    mov eax, SYS_WRITE
    mov edi, r13d
    lea rsi, [out_buf]
    mov edx, 52
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

    xor byte ptr [is_fullscreen], 1
    cmp byte ptr [is_fullscreen], 1
    jne fs_restore

    /* Set fullscreen on target monitor */
    mov edi, r14d
    call get_mon_for_ws         /* eax = monitor index */
    mov ecx, eax
    shl ecx, 4                  /* 16 bytes per monitor */
    lea rbx, [mon_geom]
    add rbx, rcx

    mov dword ptr [out_buf + 0], (7 << 16) | 12
    mov [out_buf + 4], edx
    mov dword ptr [out_buf + 8], 15
    mov eax, [rbx + 0]          /* x */
    mov [out_buf + 12], eax
    mov eax, [rbx + 4]          /* y */
    mov [out_buf + 16], eax
    mov eax, [rbx + 8]          /* w */
    mov [out_buf + 20], eax
    mov eax, [rbx + 12]         /* h */
    mov [out_buf + 24], eax

    /* X_RaiseWindow (ConfigureWindow, stack mode = Above) so the
       fullscreen window actually covers splits from other columns */
    mov dword ptr [out_buf + 28], (4 << 16) | 12
    mov [out_buf + 32], edx
    mov dword ptr [out_buf + 36], 64
    mov dword ptr [out_buf + 40], 0

    mov eax, SYS_WRITE
    mov edi, r13d
    lea rsi, [out_buf]
    mov edx, 44
    syscall
    jmp event_loop

fs_restore:
    mov edi, r14d
    call bsp_arrange_ws
    jmp event_loop

key_focus_next:
    lea rbx, [bsp_roots]
    mov rdi, [rbx + 8*r14]
    call bsp_find_other_leaf
    test rax, rax
    jz event_loop
    lea rbx, [bsp_focuses]
    mov [rbx + 8*r14], rax
    mov edi, [rax + NODE_WIN]
    call set_focus
    jmp event_loop

key_focus_prev:
    jmp key_focus_next

set_focus:
    lea r15, [out_buf]
    call set_focus_buf
    jmp flush_out_buf

/*
 * set_focus_buf(edi = win_id, r15 = write cursor)
 * Appends 12-byte X_SetInputFocus request to [r15]
 */
set_focus_buf:
    test edi, edi
    jz focus_ret
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
    mov eax, SYS_WRITE
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
    mov eax, SYS_FORK
    syscall
    test eax, eax
    jnz parent_fork

    mov eax, SYS_VFORK
    syscall
    test eax, eax
    jnz child_exit

    mov eax, SYS_CLOSE
    mov edi, r13d
    syscall

    pop rsi
    mov eax, SYS_EXECVE
    lea rdi, [sh_bin]
    mov rdx, [saved_envp]
    syscall

child_exit:
    mov eax, SYS_EXIT
    xor edi, edi
    syscall

parent_fork:
    pop rsi
    mov edi, eax
    mov eax, SYS_WAIT4
    xor esi, esi
    xor edx, edx
    xor r10, r10
    syscall
    ret

/*
 * =========================================================================
 * WORKSPACE MANAGEMENT (9 WORKSPACES)
 * =========================================================================
 */

/*
 * focus_workspace(edi = target_ws 0..8)
 * Super-Pipeline Batching: Combines hide_tree_collect + draw_all_bars_buf +
 * bsp_arrange_ws_buf + set_focus_buf into ONE SINGLE atomic sys_write syscall!
 * (Reduces workspace switching from 5 syscalls down to 1 syscall!)
 */
focus_workspace:
    push rbx
    push r12
    push r15

    mov r12d, edi               /* r12d = new_ws */
    mov ebx, r14d               /* ebx = old_ws */
    cmp r12d, ebx
    je fw_done

    lea r15, [out_buf]          /* r15 = unified batch write cursor */

    /* 1. Hide windows of old workspace (move off-screen x=-9999, y=-9999) */
    lea rax, [bsp_roots]
    mov rdi, [rax + 8*rbx]
    call hide_tree_collect

    /* 2. Switch to new workspace directly in permanent register r14! (Zero RAM) */
    mov r14d, r12d

    /* 3. Redraw top status bars across monitors into [r15] */
    call draw_all_bars_buf

    /* 4. Arrange new workspace windows on-screen into [r15]
       (No redundant MapWindow needed: windows stay mapped while off-screen!) */
    mov edi, r12d
    call bsp_arrange_ws_buf

    /* 5. Focus active window on new workspace into [r15] */
    lea rax, [bsp_focuses]
    mov rdi, [rax + 8*r12]
    test rdi, rdi
    jz fw_flush
    mov edi, [rdi + NODE_WIN]
    call set_focus_buf

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
    mov dword ptr [r15 + 0], (5 << 16) | 12  /* Opcode 12: ConfigureWindow */
    mov eax, [rdi + NODE_WIN]
    mov [r15 + 4], eax                       /* window ID */
    mov dword ptr [r15 + 8], 3               /* CWX | CWY = 3 */
    mov dword ptr [r15 + 12], -9999          /* x = -9999 (off-screen) */
    mov dword ptr [r15 + 16], -9999          /* y = -9999 (off-screen) */
    add r15, 20
    ret

htc_recurse:
    push rdi
    mov rdi, [rdi + NODE_FIRST]
    call hide_tree_collect
    pop rdi
    mov rdi, [rdi + NODE_SECOND]
    call hide_tree_collect
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
    cmp r12d, [mon_count]
    jl create_bar_loop

    /* 3. Pre-create 3 static GCs (Eliminates all runtime ChangeGC packets!):
       - bar_gc + 0: background fill GC (fg = 0x181818)
       - bar_gc + 1: active workspace digit GC (fg = 0xc2c2b0)
       - bar_gc + 2: inactive workspace digit GC (fg = 0x685742)
     */
    mov ecx, [bar_gc]
    mov edx, [bar_win + 0]

    /* GC 0: Background (24 bytes) */
    mov dword ptr [r15 + 0], (6 << 16) | 55
    mov [r15 + 4], ecx
    mov [r15 + 8], edx
    mov dword ptr [r15 + 12], 12                /* GCForeground (4) | GCBackground (8) */
    mov dword ptr [r15 + 16], 0x181818          /* fg = bg = 0x181818 */
    mov dword ptr [r15 + 20], 0x181818

    /* GC 1: Active text (24 bytes) */
    inc ecx
    mov dword ptr [r15 + 24], (6 << 16) | 55
    mov [r15 + 28], ecx
    mov [r15 + 32], edx
    mov dword ptr [r15 + 36], 12
    mov dword ptr [r15 + 40], 0xc2c2b0          /* fg = active cream/gold */
    mov dword ptr [r15 + 44], 0x181818

    /* GC 2: Inactive text (24 bytes) */
    inc ecx
    mov dword ptr [r15 + 48], (6 << 16) | 55
    mov [r15 + 52], ecx
    mov [r15 + 56], edx
    mov dword ptr [r15 + 60], 12
    mov dword ptr [r15 + 64], 0x685742          /* fg = inactive brown/gray */
    mov dword ptr [r15 + 68], 0x181818

    /* 4. SelectInput on root window (16 bytes) */
    mov dword ptr [r15 + 72], (4 << 16) | 2
    mov eax, [root_win]
    mov [r15 + 76], eax
    mov dword ptr [r15 + 80], 0x00000800        /* CWEventMask bit 11 */
    mov dword ptr [r15 + 84], 0x00180001        /* SubstructureRedirect | SubstructureNotify | KeyPress */
    add r15, 88

    /* 5. Append initial bar render and flush everything in ONE atomic sys_write! */
    call draw_all_bars_buf
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
 * Zero-ChangeGC Bar Renderer: Uses the 3 pre-created GCs (bar_gc+0, bar_gc+1, bar_gc+2)
 * so every workspace digit takes only 20 bytes (PolyText8) with ZERO ChangeGC overhead!
 * Appends directly to [r15] so it can be combined with workspace switching in 1 syscall!
 */
draw_all_bars_buf:
    push rbx
    push r12
    push r13

    xor ebx, ebx                                /* ebx = mon_idx (0 or 1) */

dab_mon_loop:
    /* 1. Clear background with PolyFillRectangle using bar_gc + 0 (20 bytes, no ChangeGC!) */
    mov dword ptr [r15 + 0], (5 << 16) | 70     /* PolyFillRectangle */
    mov eax, [bar_win + 4*rbx]
    mov [r15 + 4], eax
    mov eax, [bar_gc]                           /* bar_gc + 0 = 0x181818 */
    mov [r15 + 8], eax
    mov dword ptr [r15 + 12], 0                 /* x=0, y=0 */
    mov eax, ebx
    shl eax, 4
    mov eax, [mon_geom + rax + 8]               /* mon width */
    mov edx, BAR_HEIGHT                         /* height */
    shl edx, 16
    and eax, 0xFFFF
    or eax, edx
    mov [r15 + 16], eax
    add r15, 20

    /* 2. Loop through workspaces 0..8 and draw ONLY those assigned to mon_idx */
    xor r12d, r12d                              /* r12d = ws_idx (0..8) */
    xor r13d, r13d                              /* r13d = display slot counter for this bar */

draw_ws_mon_loop:
    /* Check if this workspace belongs to this monitor */
    movzx eax, byte ptr [ws_mon_table + r12]
    cmp eax, ebx
    jne skip_ws_mon                             /* Not on this monitor, skip! */

    /* Select pre-created GC: bar_gc + 1 (active) or bar_gc + 2 (inactive) */
    mov edx, [bar_gc]
    add edx, 2                                  /* default: bar_gc + 2 (inactive) */
    cmp r12d, r14d
    jne emit_polytext
    dec edx                                     /* active: bar_gc + 1 */

emit_polytext:
    /* PolyText8 (20 bytes) */
    mov dword ptr [r15 + 0], (5 << 16) | 74
    mov eax, [bar_win + 4*rbx]
    mov [r15 + 4], eax
    mov [r15 + 8], edx                          /* pre-configured GC! */

    /* X position: 10 + r13d * 24, Y: 13 */
    mov eax, r13d
    imul eax, 24
    add eax, 10
    mov edx, 13
    shl edx, 16
    and eax, 0xFFFF
    or eax, edx
    mov [r15 + 12], eax

    /* Text Item: len = 1, delta = 0, char = '1' + r12d, pad = 0 */
    mov ecx, r12d
    add ecx, '1'
    shl ecx, 16
    or ecx, 1                                   /* len = 1 in low byte */
    mov [r15 + 16], ecx

    add r15, 20
    inc r13d                                    /* Advance slot counter */

skip_ws_mon:
    inc r12d
    cmp r12d, 9
    jl draw_ws_mon_loop

    inc ebx
    cmp ebx, [mon_count]
    jl dab_mon_loop

    pop r13
    pop r12
    pop rbx
    ret

/*
 * =========================================================================
 * DUAL MONITOR DETECTION VIA XINERAMA EXTENSION
 * =========================================================================
 */
detect_monitors:
    push rbx

    /* Default: 1 monitor = full screen */
    mov dword ptr [mon_count], 1
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

    mov eax, SYS_WRITE
    mov edi, r13d
    lea rsi, [out_buf]
    mov edx, 16
    syscall

    /* Read reply (32 bytes) */
    mov eax, SYS_READ
    mov edi, r13d
    lea rsi, [in_buf]
    mov edx, 32
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

    mov eax, SYS_WRITE
    mov edi, r13d
    lea rsi, [out_buf]
    mov edx, 4
    syscall

    /* Read reply header (32 bytes) */
    mov eax, SYS_READ
    mov edi, r13d
    lea rsi, [in_buf]
    mov edx, 32
    syscall

    cmp byte ptr [in_buf], 1
    jne mon_detect_done

    mov ecx, [in_buf + 8]               /* number of screens */
    cmp ecx, 2
    jl mon_detect_done

    /* 2 monitors detected! Read 2 x 8 bytes monitor structures */
    mov eax, SYS_READ
    mov edi, r13d
    lea rsi, [in_buf]
    mov edx, 16                         /* 2 screens * 8 bytes */
    syscall

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

    mov dword ptr [mon_count], 2

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

    mov eax, SYS_WRITE
    mov edi, r13d
    lea rsi, [out_buf]
    mov edx, 44
    syscall

    /* Read both 32-byte replies (64 bytes total) */
    mov eax, SYS_READ
    mov edi, r13d
    lea rsi, [in_buf]
    mov edx, 64
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
    cmp dword ptr [mon_count], 2
    jl single_mon
    cmp edi, WORKSPACE_COUNT
    jge single_mon
    movzx eax, byte ptr [ws_mon_table + rdi]
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

    /* Batch MapWindow + set_focus + bsp_arrange_ws into ONE atomic syscall! */
    lea r15, [out_buf]
    mov dword ptr [r15 + 0], (2 << 16) | 8
    mov dword ptr [r15 + 4], r12d
    add r15, 8

    mov edi, r12d
    call set_focus_buf

    mov edi, r14d
    call bsp_arrange_ws_buf
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
    mov edi, [rax + NODE_WIN]
    call set_focus_buf
no_focus_left:
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
    mov [rax + NODE_WS], r14b
    mov qword ptr [rax + NODE_FIRST], 0
    mov qword ptr [rax + NODE_SECOND], 0
    mov qword ptr [rax + NODE_PARENT], 0
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
    mov [r13 + NODE_WS], r14b

    /* Determine split direction: 1 = V, 2 = H, 0 = auto */
    movzx ecx, byte ptr [next_split_sel]
    test cl, cl
    jnz ws_use_chosen

    /* Alternate split */
    mov rax, [rbx + NODE_PARENT]
    test rax, rax
    jz ws_def_v
    movzx ecx, byte ptr [rax + NODE_SPLIT]
    xor cl, 1
    jmp ws_apply_split
ws_def_v:
    xor cl, cl
    jmp ws_apply_split

ws_use_chosen:
    dec cl                      /* 1 -> 0 (V), 2 -> 1 (H) */
    mov byte ptr [next_split_sel], 0

ws_apply_split:
    mov dword ptr [r13 + NODE_WIN], 0
    mov [r13 + NODE_SPLIT], cl
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
    movzx r14d, byte ptr [rbx + NODE_WS]    /* ws_idx */
    mov r12, [rbx + NODE_PARENT]
    test r12, r12
    jnz td_has_parent

    /* Removing root */
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
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

/*
 * alloc_node() -> rax = pointer to new 32-byte aligned node
 */
alloc_node:
    mov eax, [node_idx]
    cmp eax, 15
    jge alloc_fail
    inc dword ptr [node_idx]
    shl rax, 5                  /* rax * 32 bytes */
    lea rdx, [node_pool]
    add rax, rdx
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

bsp_find_other_leaf:
    test rdi, rdi
    jz bfol_null
bfol_loop:
    cmp dword ptr [rdi + NODE_WIN], 0
    je bfol_down
    lea rbx, [bsp_focuses]
    cmp rdi, [rbx + 8*r14]
    jne bfol_found
    xor eax, eax
    ret
bfol_down:
    push rdi
    mov rdi, [rdi + NODE_FIRST]
    call bsp_find_other_leaf
    pop rdi
    test rax, rax
    jnz bfol_ret
    mov rdi, [rdi + NODE_SECOND]
    call bsp_find_other_leaf
    ret
bfol_found:
    mov rax, rdi
    ret
bfol_null:
    xor eax, eax
bfol_ret:
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
    call bsp_find_node_by_win
    ret

bfn_found:
    mov rax, rdi
    ret
bfn_null:
    xor eax, eax
bfn_ret:
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
 * Writes ConfigureWindow packets to [r15] cursor.
 */
bsp_arrange_node:
    push rbp
    mov rbp, rsp
    sub rsp, 48

    mov [rbp - 8],  rdi
    mov [rbp - 16], rsi
    mov [rbp - 24], rdx
    mov [rbp - 32], r8
    mov [rbp - 40], r9

    cmp dword ptr [rdi + NODE_WIN], 0
    jz do_ws_split

    /* Configure leaf window: append 28-byte packet to [r15] */
    mov dword ptr [r15 + 0], (7 << 16) | 12
    mov eax, [rdi + NODE_WIN]
    mov [r15 + 4], eax
    mov dword ptr [r15 + 8], 15
    mov eax, [rbp - 16]
    mov [r15 + 12], eax
    mov eax, [rbp - 24]
    mov [r15 + 16], eax
    mov eax, [rbp - 32]
    mov [r15 + 20], eax
    mov eax, [rbp - 40]
    mov [r15 + 24], eax
    add r15, 28
    jmp arrange_ret

do_ws_split:
    cmp byte ptr [rdi + NODE_SPLIT], 1
    je ws_split_horizontal

    /* Vertical Split */
    mov r8, [rbp - 32]
    shr r8, 1
    sub r8, GAP_PX / 2          /* w1 */

    mov rdi, [rbp - 8]
    mov rdi, [rdi + NODE_FIRST]
    mov rsi, [rbp - 16]
    mov rdx, [rbp - 24]
    mov r9,  [rbp - 40]
    call bsp_arrange_node

    mov rdi, [rbp - 8]
    mov rdi, [rdi + NODE_SECOND]
    mov rsi, [rbp - 16]
    mov r8,  [rbp - 32]
    shr r8, 1
    sub r8, GAP_PX / 2
    add rsi, r8
    add rsi, GAP_PX             /* x2 = x + w1 + gap */
    mov rdx, [rbp - 24]
    mov r8,  [rbp - 32]
    sub r8, rsi
    add r8,  [rbp - 16]         /* w2 */
    mov r9,  [rbp - 40]
    call bsp_arrange_node
    jmp arrange_ret

ws_split_horizontal:
    /* Horizontal Split */
    mov r9, [rbp - 40]
    shr r9, 1
    sub r9, GAP_PX / 2          /* h1 */

    mov rdi, [rbp - 8]
    mov rdi, [rdi + NODE_FIRST]
    mov rsi, [rbp - 16]
    mov rdx, [rbp - 24]
    mov r8,  [rbp - 32]
    call bsp_arrange_node

    mov rdi, [rbp - 8]
    mov rdi, [rdi + NODE_SECOND]
    mov rsi, [rbp - 16]
    mov rdx, [rbp - 24]
    mov r9,  [rbp - 40]
    shr r9, 1
    sub r9, GAP_PX / 2
    add rdx, r9
    add rdx, GAP_PX             /* y2 = y + h1 + gap */
    mov r8,  [rbp - 32]
    mov r9,  [rbp - 40]
    sub r9, rdx
    add r9,  [rbp - 24]
    call bsp_arrange_node

arrange_ret:
    leave
    ret

/*
 * grab_all_keys:
 * Batches all X_GrabKey requests into `in_buf` (4096 bytes) and issues
 * exactly ONE atomic `sys_write` syscall instead of 72 individual syscalls!
 */
grab_all_keys:
    push rbx
    push r12
    push r13

    lea rbx, [shortcut_keys]
    lea r12, [in_buf]           /* destination write pointer */
    mov r13d, [root_win]

grab_batch_loop:
    movzx edi, byte ptr [rbx]   /* keycode */
    test edi, edi
    jz grab_batch_send
    movzx esi, byte ptr [rbx + 1] /* modifiers */

    /* Emit standard combo */
    mov dword ptr [r12 + 0], (4 << 16) | (1 << 8) | 33
    mov [r12 + 4], r13d
    mov eax, edi
    shl eax, 16
    and esi, 0xFFFF
    or eax, esi
    or eax, (1 << 24)
    mov [r12 + 8], eax
    mov dword ptr [r12 + 12], 1
    add r12, 16

    /* Emit NumLock/CapsLock combo (modifiers | 0x10) */
    mov dword ptr [r12 + 0], (4 << 16) | (1 << 8) | 33
    mov [r12 + 4], r13d
    or eax, 0x10                /* add NumLock mask to modifiers word */
    mov [r12 + 8], eax
    mov dword ptr [r12 + 12], 1
    add r12, 16

    add rbx, 2
    jmp grab_batch_loop

grab_batch_send:
    /* Calculate total bytes: r12 - in_buf */
    pop r13                     /* restore permanent socket fd in r13d! */
    lea rax, [in_buf]
    sub r12, rax                /* r12 = total byte count */
    jz grab_batch_done

    mov eax, SYS_WRITE
    mov edi, r13d
    lea rsi, [in_buf]
    mov edx, r12d
    syscall

grab_batch_done:
    pop r12
    pop rbx
    ret


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

    /* Check parent split orientation */
    movzx eax, byte ptr [rbx + NODE_SPLIT]
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

    movzx eax, byte ptr [r15 + NODE_SPLIT]
    cmp eax, r13d
    je dive_same_orient

    /* Perpendicular split: take first child */
    mov r15, [r15 + NODE_FIRST]
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
    mov eax, SYS_EXIT
    xor edi, edi
    syscall

exit_fail:
    call kill_xorg
    mov eax, SYS_EXIT
    mov edi, 1
    syscall

/* kill_xorg: if we spawned Xorg, SIGTERM it and reap it */
kill_xorg:
    mov rdi, [xorg_pid]
    test rdi, rdi
    jle 1f
    mov eax, 62                 /* SYS_KILL */
    mov esi, 15                 /* SIGTERM */
    syscall
    mov rdi, [xorg_pid]
    mov eax, SYS_WAIT4
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
    mov eax, 89                 /* SYS_READLINK */
    lea rdi, [proc_fd0]
    lea rsi, [tty_buf]
    mov edx, 31
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
2:  mov eax, SYS_FORK
    syscall
    test rax, rax
    jnz 5f
    lea rdi, [x_path]
    lea rsi, [x_argv]
    mov rdx, [saved_envp]
    mov eax, SYS_EXECVE
    syscall
    mov eax, SYS_EXIT
    mov edi, 1
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
    .asciz "st || alacritty || kitty"

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
 * Workspace to Monitor Mapping Configuration Table:
 * Define which monitor (0 or 1) hosts each workspace (WS 1 to 9).
 * Index 0 = Workspace 1, Index 1 = Workspace 2, ..., Index 8 = Workspace 9.
 * (Change the bytes below to customize your dual-monitor setup!)
 */
.align 8
ws_mon_table:
    .byte 1                     /* WS 1 -> Monitor 1 */
    .byte 1                     /* WS 2 -> Monitor 1 */
    .byte 1                     /* WS 3 -> Monitor 1 */
    .byte 1                     /* WS 4 -> Monitor 1 */
    .byte 1                     /* WS 5 -> Monitor 1 */
    .byte 1                     /* WS 6 -> Monitor 1 */
    .byte 1                     /* WS 7 -> Monitor 1 */
    .byte 1                     /* WS 8 -> Monitor 1 */
    .byte 0                     /* WS 9 -> Monitor 0 */

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
disp_found:     .byte 0
/* cur_ws lives permanently in register r14 (Zero RAM!) */
next_split_sel: .byte 0         /* 0 = auto, 1 = V, 2 = H - 1 byte! */
is_fullscreen:  .byte 0         /* 0 = normal, 1 = fullscreen - 1 byte! */
                .align 4

/* Monitor configuration: up to 2 monitors (16 bytes each: x, y, w, h) */
mon_count:      .long 0
mon_geom:       .space 32

/* 9 Workspaces state */
bsp_roots:      .space 72       /* 9 x 8 bytes pointers */
bsp_focuses:    .space 72       /* 9 x 8 bytes pointers */

node_idx:       .long 0
ev_q_head:      .long 0         /* Current read offset in event_queue */
ev_q_tail:      .long 0         /* Total bytes available in event_queue */

/*
 * Shared I/O Buffer (832 bytes total - compacted so the ENTIRE hot binary
 * (.text + .data + hot .bss) fits inside 2 physical 4 KB pages [0x400000..0x401fff]!):
 * - `out_buf` uses first 544 bytes (during startup `grab_all_keys` uses 1152B across in_buf+event_queue+node_pool)
 * - `event_queue` uses next 288 bytes (9 X11 events * 32 bytes - zero-copy in-place dispatch!)
 */
.align 32
out_buf:
in_buf:         .space 544
event_queue:    .space 288      /* Buffer for up to 9 X11 events (32 bytes each) */

/* 16 nodes of 32 bytes = 512 bytes (fits hot .bss completely inside page 0x401000..0x401fff!) */
.align 32
node_pool:      .space 512

/* Cold TTY-only buffers placed at very end of .bss so they are never faulted in X sessions */
env_buf:        .space 256
tty_buf:        .space 32
