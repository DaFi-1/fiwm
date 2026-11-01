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

/*
 * Node Structure (64 bytes aligned):
 * offset 0:  qword client_win (if leaf) OR 0 (if internal split node)
 * offset 8:  qword first_child (pointer)
 * offset 16: qword second_child (pointer)
 * offset 24: qword parent (pointer)
 * offset 32: qword split_type (0 = vertical, 1 = horizontal)
 * offset 40: qword ws_idx (0..8)
 */
.equ NODE_WIN,     0
.equ NODE_FIRST,   8
.equ NODE_SECOND,  16
.equ NODE_PARENT,  24
.equ NODE_SPLIT,   32
.equ NODE_WS,      40
.equ NODE_SIZE,    64

.text
_start:
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
    /* 1. sys_socket(AF_UNIX, SOCK_STREAM, 0) */
    mov eax, SYS_SOCKET
    mov edi, AF_UNIX
    mov esi, SOCK_STREAM
    xor edx, edx
    syscall
    test eax, eax
    js exit_fail
    mov [sock_fd], eax
    mov ebx, eax

    /* 2. sys_connect(sock_fd, &sock_addr, sizeof(sock_addr)) */
    mov eax, SYS_CONNECT
    mov edi, [sock_fd]
    lea rsi, [sock_addr]
    mov edx, 110
    syscall
    test eax, eax
    js exit_fail

    /* 3. Send X11 Setup Request (12 bytes) */
    mov eax, SYS_WRITE
    mov edi, [sock_fd]
    lea rsi, [x11_setup_req]
    mov edx, 12
    syscall

    /* 4. Read X11 Setup Reply Header (8 bytes) */
    mov eax, SYS_READ
    mov edi, [sock_fd]
    lea rsi, [in_buf]
    mov edx, 8
    syscall
    cmp byte ptr [in_buf], 1
    jne exit_fail

    movzx ecx, word ptr [in_buf + 6]
    shl ecx, 2

    mov eax, SYS_READ
    mov edi, [sock_fd]
    lea rsi, [in_buf]
    mov edx, ecx
    syscall

    /* Parse Screen Info */
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

    /* Detect Dual Monitor (Xinerama) */
    call detect_monitors

    /* 5. SelectInput on root window (SubstructureRedirect | SubstructureNotify | KeyPress) */
    mov dword ptr [out_buf + 0], (4 << 16) | 2
    mov eax, [root_win]
    mov dword ptr [out_buf + 4], eax
    mov dword ptr [out_buf + 8], 0x00000800  /* CWEventMask bit 11 */
    mov dword ptr [out_buf + 12], 0x00180001 /* SubstructureRedirect | SubstructureNotify | KeyPress */

    mov eax, SYS_WRITE
    mov edi, [sock_fd]
    lea rsi, [out_buf]
    mov edx, 16
    syscall

    /* 6. Grab Shortcut Keys (including Alt+1..9 and Alt+Shift+1..9) */
    call grab_all_keys

event_loop:
    mov eax, SYS_READ
    mov edi, [sock_fd]
    lea rsi, [event_buf]
    mov edx, 32
    syscall

    test eax, eax
    jle exit_clean

    movzx eax, byte ptr [event_buf]
    and al, 0x7F

    cmp al, 2                   /* KeyPress */
    je handle_key

    cmp al, 20                  /* MapRequest */
    je handle_map

    cmp al, 18                  /* UnmapNotify */
    je handle_unmap

    cmp al, 17                  /* DestroyNotify */
    je handle_unmap

    jmp event_loop

/*
 * =========================================================================
 * KEY EVENT HANDLERS
 * =========================================================================
 */
handle_key:
    movzx ecx, byte ptr [event_buf + 1]  /* keycode */
    movzx edx, word ptr [event_buf + 28] /* state/modifiers */

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

    /* Alt+j (44): Focus Next */
    cmp ecx, 44
    je key_focus_next

    /* Alt+k (45): Focus Prev */
    cmp ecx, 45
    je key_focus_prev

    /* Alt+v (55): Select Next Split = Horizontal for focused window */
    cmp ecx, 55
    je key_split_h

    /* Alt+b (56): Select Next Split = Vertical for focused window */
    cmp ecx, 56
    je key_split_v

    jmp event_loop

key_split_h:
    mov dword ptr [next_split_sel], 2   /* 2 = horizontal */
    jmp event_loop

key_split_v:
    mov dword ptr [next_split_sel], 1   /* 1 = vertical */
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
    mov eax, [cur_ws]
    lea rbx, [bsp_focuses]
    mov rax, [rbx + 8*rax]
    test rax, rax
    jz event_loop
    mov edx, [rax + NODE_WIN]
    test edx, edx
    jz event_loop

    /* Opcode 4: DestroyWindow (len 2 words = 8 bytes) */
    mov dword ptr [out_buf + 0], (2 << 16) | 4
    mov [out_buf + 4], edx

    mov eax, SYS_WRITE
    mov edi, [sock_fd]
    lea rsi, [out_buf]
    mov edx, 8
    syscall
    jmp event_loop

key_fullscreen:
    mov eax, [cur_ws]
    lea rbx, [bsp_focuses]
    mov rax, [rbx + 8*rax]
    test rax, rax
    jz event_loop
    mov edx, [rax + NODE_WIN]
    test edx, edx
    jz event_loop

    xor dword ptr [is_fullscreen], 1
    cmp dword ptr [is_fullscreen], 1
    jne fs_restore

    /* Set fullscreen on target monitor */
    mov edi, [cur_ws]
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

    mov eax, SYS_WRITE
    mov edi, [sock_fd]
    lea rsi, [out_buf]
    mov edx, 28
    syscall
    jmp event_loop

fs_restore:
    mov edi, [cur_ws]
    call bsp_arrange_ws
    jmp event_loop

key_focus_next:
    mov eax, [cur_ws]
    lea rbx, [bsp_roots]
    mov rdi, [rbx + 8*rax]
    call bsp_find_other_leaf
    test rax, rax
    jz event_loop
    mov ecx, [cur_ws]
    lea rbx, [bsp_focuses]
    mov [rbx + 8*rcx], rax
    mov edi, [rax + NODE_WIN]
    call set_focus
    jmp event_loop

key_focus_prev:
    jmp key_focus_next

set_focus:
    test edi, edi
    jz focus_ret
    mov dword ptr [out_buf + 0], (3 << 16) | (1 << 8) | 42
    mov [out_buf + 4], edi
    mov dword ptr [out_buf + 8], 0
    mov eax, SYS_WRITE
    mov edi, [sock_fd]
    lea rsi, [out_buf]
    mov edx, 12
    syscall
focus_ret:
    ret

/*
 * spawn_cmd(rsi = argv_pointer)
 */
spawn_cmd:
    push rsi
    mov eax, SYS_FORK
    syscall
    test eax, eax
    jnz parent_fork

    mov eax, SYS_FORK
    syscall
    test eax, eax
    jnz child_exit

    mov eax, SYS_CLOSE
    mov edi, [sock_fd]
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
 */
focus_workspace:
    push rbx
    push r12

    mov r12d, edi               /* r12d = new_ws */
    mov ebx, [cur_ws]           /* ebx = old_ws */
    cmp r12d, ebx
    je fw_done

    /* Unmap (hide) windows of old workspace */
    lea rax, [bsp_roots]
    mov rdi, [rax + 8*rbx]
    call hide_tree_windows

    /* Switch to new workspace */
    mov [cur_ws], r12d

    /* Map (show) windows of new workspace */
    lea rax, [bsp_roots]
    mov rdi, [rax + 8*r12]
    call show_tree_windows

    /* Arrange new workspace */
    mov edi, r12d
    call bsp_arrange_ws

    /* Focus active window on new workspace */
    lea rax, [bsp_focuses]
    mov rdi, [rax + 8*r12]
    test rdi, rdi
    jz fw_done
    mov edi, [rdi + NODE_WIN]
    call set_focus

fw_done:
    pop r12
    pop rbx
    ret

/*
 * move_window_to_workspace(edi = target_ws 0..8)
 */
move_window_to_workspace:
    push rbx
    push r12

    mov r12d, edi               /* target_ws */
    mov eax, [cur_ws]
    cmp r12d, eax
    je mw_done

    /* Get current focused leaf */
    lea rbx, [bsp_focuses]
    mov rdi, [rbx + 8*rax]
    test rdi, rdi
    jz mw_done

    mov ebx, [rdi + NODE_WIN]   /* ebx = win */
    test ebx, ebx
    jz mw_done

    /* Detach from current workspace */
    push rdi
    call bsp_tree_detach
    pop rdi

    /* Hide window from current view */
    mov edi, [sock_fd]
    call hide_single_win

    /* Re-arrange current workspace */
    mov edi, [cur_ws]
    call bsp_arrange_ws

    /* Insert window into target workspace tree */
    mov edi, [sock_fd]
    mov esi, r12d
    call bsp_tree_insert_into_ws

mw_done:
    pop r12
    pop rbx
    ret

/*
 * hide_tree_windows(rdi = node)
 */
hide_tree_windows:
    test rdi, rdi
    jz htw_ret
    cmp qword ptr [rdi + NODE_WIN], 0
    je htw_recurse
    mov edi, [rdi + NODE_WIN]
    call hide_single_win
    ret
htw_recurse:
    push rdi
    mov rdi, [rdi + NODE_FIRST]
    call hide_tree_windows
    pop rdi
    mov rdi, [rdi + NODE_SECOND]
    call hide_tree_windows
htw_ret:
    ret

hide_single_win:
    push rcx
    push r11
    mov dword ptr [out_buf + 0], (2 << 16) | 10
    mov [out_buf + 4], edi
    mov eax, SYS_WRITE
    mov edi, [sock_fd]
    lea rsi, [out_buf]
    mov edx, 8
    syscall
    pop r11
    pop rcx
    ret

/*
 * show_tree_windows(rdi = node)
 */
show_tree_windows:
    test rdi, rdi
    jz stw_ret
    cmp qword ptr [rdi + NODE_WIN], 0
    je stw_recurse
    mov edi, [rdi + NODE_WIN]
    call show_single_win
    ret
stw_recurse:
    push rdi
    mov rdi, [rdi + NODE_FIRST]
    call show_tree_windows
    pop rdi
    mov rdi, [rdi + NODE_SECOND]
    call show_tree_windows
stw_ret:
    ret

show_single_win:
    push rcx
    push r11
    mov dword ptr [out_buf + 0], (2 << 16) | 8
    mov [out_buf + 4], edi
    mov eax, SYS_WRITE
    mov edi, [sock_fd]
    lea rsi, [out_buf]
    mov edx, 8
    syscall
    pop r11
    pop rcx
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
    mov edi, [sock_fd]
    lea rsi, [out_buf]
    mov edx, 16
    syscall

    /* Read reply (32 bytes) */
    mov eax, SYS_READ
    mov edi, [sock_fd]
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
    mov edi, [sock_fd]
    lea rsi, [out_buf]
    mov edx, 4
    syscall

    /* Read reply header (32 bytes) */
    mov eax, SYS_READ
    mov edi, [sock_fd]
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
    mov edi, [sock_fd]
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
    pop rbx
    ret

/*
 * get_mon_for_ws(edi = ws_idx) -> eax = monitor index (0 or 1)
 */
get_mon_for_ws:
    cmp dword ptr [mon_count], 2
    jl single_mon
    /* Monitor 1 gets WS 8 (Workspace 9), Monitor 0 gets WS 0..7 */
    cmp edi, 8
    je is_mon1
single_mon:
    xor eax, eax
    ret
is_mon1:
    mov eax, 1
    ret

/*
 * =========================================================================
 * TRUE HIERARCHICAL BSP TREE ENGINE
 * =========================================================================
 */

handle_map:
    mov edx, [event_buf + 8]
    test edx, edx
    jz event_loop
    mov r12d, edx

    /* MapWindow */
    mov dword ptr [out_buf + 0], (2 << 16) | 8
    mov dword ptr [out_buf + 4], r12d
    mov eax, SYS_WRITE
    mov edi, [sock_fd]
    lea rsi, [out_buf]
    mov edx, 8
    syscall

    /* Insert into active workspace BSP tree */
    mov edi, r12d
    mov esi, [cur_ws]
    call bsp_tree_insert_into_ws

    /* Focus new window */
    mov edi, r12d
    call set_focus

    mov edi, [cur_ws]
    call bsp_arrange_ws
    jmp event_loop

handle_unmap:
    mov edx, [event_buf + 8]
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

    /* If it was in active workspace, re-arrange */
    cmp ecx, [cur_ws]
    jne event_loop

    lea rax, [bsp_roots]
    mov rdi, [rax + 8*rcx]
    call bsp_first_leaf
    lea rbx, [bsp_focuses]
    mov [rbx + 8*rcx], rax
    test rax, rax
    jz no_focus_left
    mov edi, [rax + NODE_WIN]
    call set_focus
no_focus_left:
    mov edi, [cur_ws]
    call bsp_arrange_ws
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
    mov [rax + NODE_WIN], rdi
    mov [rax + NODE_WS], r14
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
    mov [r13 + NODE_WS], r14

    /* Determine split direction: 1 = V, 2 = H, 0 = auto */
    mov ecx, [next_split_sel]
    test ecx, ecx
    jnz ws_use_chosen

    /* Alternate split */
    mov rax, [rbx + NODE_PARENT]
    test rax, rax
    jz ws_def_v
    mov rcx, [rax + NODE_SPLIT]
    xor rcx, 1
    jmp ws_apply_split
ws_def_v:
    xor ecx, ecx
    jmp ws_apply_split

ws_use_chosen:
    dec ecx                     /* 1 -> 0 (V), 2 -> 1 (H) */
    mov dword ptr [next_split_sel], 0

ws_apply_split:
    mov qword ptr [r13 + NODE_WIN], 0
    mov [r13 + NODE_SPLIT], rcx
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
    mov r14, [rbx + NODE_WS]    /* ws_idx */
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
 * alloc_node() -> rax = pointer to new 64-byte aligned node
 */
alloc_node:
    mov eax, [node_idx]
    cmp eax, 63
    jge alloc_fail
    inc dword ptr [node_idx]
    shl rax, 6                  /* rax * 64 bytes */
    lea rdx, [node_pool]
    add rax, rdx
    mov qword ptr [rax + 0],  0
    mov qword ptr [rax + 8],  0
    mov qword ptr [rax + 16], 0
    mov qword ptr [rax + 24], 0
    mov qword ptr [rax + 32], 0
    mov qword ptr [rax + 40], 0
    ret
alloc_fail:
    lea rax, [node_pool]
    ret

bsp_first_leaf:
    test rdi, rdi
    jz bfl_null
bfl_loop:
    cmp qword ptr [rdi + NODE_WIN], 0
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
    cmp qword ptr [rdi + NODE_WIN], 0
    je bfol_down
    mov eax, [cur_ws]
    lea rbx, [bsp_focuses]
    cmp rdi, [rbx + 8*rax]
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
    cmp qword ptr [rdi + NODE_WIN], 0
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
 * Arranges workspace onto its specific monitor geometry!
 */
bsp_arrange_ws:
    push rbx
    push r12
    push r13
    push r14
    push r15

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

    mov rdi, r13                /* rdi = root node (RESTORED!) */
    mov esi, [rbx + 0]
    add esi, GAP_PX             /* x */
    mov edx, [rbx + 4]
    add edx, GAP_PX             /* y */
    mov r8d, [rbx + 8]
    sub r8d, GAP_PX * 2         /* w */
    mov r9d, [rbx + 12]
    sub r9d, GAP_PX * 2         /* h */

    call bsp_arrange_node

arrange_ws_done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

/*
 * bsp_arrange_node(rdi=node, esi=x, edx=y, r8=w, r9=h)
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

    cmp qword ptr [rdi + NODE_WIN], 0
    jz do_ws_split

    /* Configure leaf window */
    mov eax, [rdi + NODE_WIN]
    mov [out_buf + 4], eax
    mov dword ptr [out_buf + 0], (7 << 16) | 12
    mov dword ptr [out_buf + 8], 15
    mov eax, [rbp - 16]
    mov [out_buf + 12], eax
    mov eax, [rbp - 24]
    mov [out_buf + 16], eax
    mov eax, [rbp - 32]
    mov [out_buf + 20], eax
    mov eax, [rbp - 40]
    mov [out_buf + 24], eax

    push rcx
    push r11
    mov eax, SYS_WRITE
    mov edi, [sock_fd]
    lea rsi, [out_buf]
    mov edx, 28
    syscall
    pop r11
    pop rcx
    jmp arrange_ret

do_ws_split:
    mov rax, [rdi + NODE_SPLIT]
    cmp rax, 1
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
 * Helper: Grab key on root window
 */
grab_one_key:
    mov dword ptr [out_buf + 0], (4 << 16) | (1 << 8) | 33
    mov eax, [root_win]
    mov dword ptr [out_buf + 4], eax
    shl edi, 16
    and esi, 0xFFFF
    or edi, esi
    or edi, (1 << 24)
    mov [out_buf + 8], edi
    mov dword ptr [out_buf + 12], 1

    mov eax, SYS_WRITE
    mov edi, [sock_fd]
    lea rsi, [out_buf]
    mov edx, 16
    syscall
    ret

grab_all_keys:
    push rbx
    lea rbx, [shortcut_keys]
grab_loop:
    movzx rdi, byte ptr [rbx]
    test rdi, rdi
    jz grab_done
    movzx rsi, byte ptr [rbx + 1]
    push rbx
    call grab_one_key
    /* Also grab with CapsLock / NumLock */
    movzx rdi, byte ptr [rbx]
    movzx rsi, byte ptr [rbx + 1]
    or esi, 0x10
    call grab_one_key
    pop rbx
    add rbx, 2
    jmp grab_loop
grab_done:
    pop rbx
    ret

exit_clean:
    mov eax, SYS_EXIT
    xor edi, edi
    syscall

exit_fail:
    mov eax, SYS_EXIT
    mov edi, 1
    syscall

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

sh_bin:
    .asciz "/bin/sh"
sh_arg_c:
    .asciz "-c"

cmd_menu:
    .asciz "[ -x $HOME/dotfiles/scripts/nviciod.sh ] && $HOME/dotfiles/scripts/nviciod.sh || ([ -x $HOME/dotfile/dmenuscript ] && $HOME/dotfile/dmenuscript || st)"

cmd_term:
    .asciz "st || alacritty || kitty"

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

/*
 * Table of Grabbed Shortcuts:
 * [Keycode, Modifiers]
 */
shortcut_keys:
    /* Basic WM shortcuts */
    .byte 40, 0x08              /* Alt + d */
    .byte 25, 0x08              /* Alt + w */
    .byte 36, 0x08              /* Alt + Return */
    .byte 24, 0x08              /* Alt + q */
    .byte 24, 0x09              /* Alt + Shift + q */
    .byte 41, 0x08              /* Alt + f */
    .byte 44, 0x08              /* Alt + j */
    .byte 45, 0x08              /* Alt + k */
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
sock_fd:        .long 0
root_win:       .long 0
screen_w:       .long 0
screen_h:       .long 0
saved_envp:     .quad 0

cur_ws:         .long 0         /* Active Workspace (0..8) */
next_split_sel: .long 0         /* 0 = auto, 1 = V, 2 = H */
is_fullscreen:  .long 0

/* Monitor configuration: up to 2 monitors (16 bytes each: x, y, w, h) */
mon_count:      .long 0
mon_geom:       .space 32

/* 9 Workspaces state */
bsp_roots:      .space 72       /* 9 x 8 bytes pointers */
bsp_focuses:    .space 72       /* 9 x 8 bytes pointers */

node_idx:       .long 0

/* 64 nodes of 64 bytes = 4096 bytes */
.align 64
node_pool:      .space 4096

out_buf:        .space 64
in_buf:         .space 4096
event_buf:      .space 64
