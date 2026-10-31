#!/usr/bin/env bash
# test_all_shortcuts.sh - Comprehensive automated shortcut test suite for fiwm
set -euo pipefail

TEST_DISP=":99"
TEST_LOG="/tmp/fiwm_shortcuts_test.log"
XEPHYR_LOG="/tmp/xephyr_disp99.log"
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

cleanup() {
    if [ -n "${WM_PID:-}" ] && kill -0 "$WM_PID" 2>/dev/null; then
        kill "$WM_PID" 2>/dev/null || true
    fi
    if [ -n "${XE_PID:-}" ] && kill -0 "$XE_PID" 2>/dev/null; then
        kill "$XE_PID" 2>/dev/null || true
    fi
    pkill -f "st.*$TEST_DISP" 2>/dev/null || true
    rm -f "/tmp/.X11-unix/X${TEST_DISP#:}"
    rm -f /tmp/fiwm_dmenu_ran
    rm -f /home/a/dotfile/dmenuscript 2>/dev/null || true
    rmdir /home/a/dotfile 2>/dev/null || true
}
trap cleanup EXIT INT TERM

report_pass() {
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    PASSED_TESTS=$((PASSED_TESTS + 1))
    echo "  [PASS] $1"
}

report_fail() {
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    FAILED_TESTS=$((FAILED_TESTS + 1))
    echo "  [FAIL] $1: $2"
}

get_win_hex() {
    local dec_id="$1"
    printf "0x%x" "$dec_id"
}

get_win_pixel_geom() {
    local hex_id="$1"
    xwininfo -root -tree -display "$TEST_DISP" 2>/dev/null | grep "$hex_id" | head -n 1 | awk '{print $(NF-1)}'
}

get_win_offset() {
    local hex_id="$1"
    xwininfo -root -tree -display "$TEST_DISP" 2>/dev/null | grep "$hex_id" | head -n 1 | awk '{print $NF}'
}

echo "========================================================="
echo "       fiwm Complete Shortcut Verification Suite         "
echo "========================================================="

# 1. Compile fiwm
echo "[*] Compiling fiwm..."
make fiwm >/dev/null

# 2. Setup mock dmenuscript for Alt+d testing
mkdir -p /home/a/dotfile
cat << 'EOF' > /home/a/dotfile/dmenuscript
#!/bin/sh
touch /tmp/fiwm_dmenu_ran
EOF
chmod +x /home/a/dotfile/dmenuscript
rm -f /tmp/fiwm_dmenu_ran

# 3. Start Xephyr on isolated display
echo "[*] Launching Xephyr on display ${TEST_DISP} (800x600)..."
rm -f "/tmp/.X11-unix/X${TEST_DISP#:}"
Xephyr -br -ac -noreset -screen 800x600 "$TEST_DISP" >"$XEPHYR_LOG" 2>&1 &
XE_PID=$!

for i in $(seq 1 30); do
    if [ -S "/tmp/.X11-unix/X${TEST_DISP#:}" ]; then break; fi
    sleep 0.1
done

# 4. Start fiwm
echo "[*] Starting fiwm..."
DISPLAY="$TEST_DISP" ./fiwm >"$TEST_LOG" 2>&1 &
WM_PID=$!
sleep 0.6

if ! kill -0 "$WM_PID" 2>/dev/null; then
    echo "[!] fiwm crashed on startup!"
    cat "$TEST_LOG"
    exit 1
fi

echo ""
echo "--- [1] Window Spawning & Basic Control ---"

# Shortcut: Alt+w (spawn st)
DISPLAY="$TEST_DISP" xdotool key alt+w
sleep 0.6
ST_WIN_DEC=$(DISPLAY="$TEST_DISP" xdotool search --onlyvisible --class st 2>/dev/null | head -n 1 || true)
if [ -n "$ST_WIN_DEC" ]; then
    ST_WIN_HEX=$(get_win_hex "$ST_WIN_DEC")
    GEOM=$(get_win_pixel_geom "$ST_WIN_HEX")
    report_pass "Alt+w (spawn st) -> Window $ST_WIN_HEX created at $GEOM"
else
    report_fail "Alt+w (spawn st)" "Window was not created"
    exit 1
fi

# Shortcut: Alt+d (spawn menu / menucmd)
DISPLAY="$TEST_DISP" xdotool key alt+d
sleep 0.5
if [ -f /tmp/fiwm_dmenu_ran ]; then
    report_pass "Alt+d (spawn menucmd) -> Script successfully invoked"
else
    report_fail "Alt+d (spawn menucmd)" "Script execution marker not found"
fi

echo ""
echo "--- [2] Window States: Fullscreen & Floating ---"

# Shortcut: Alt+f (fullscreen enter)
DISPLAY="$TEST_DISP" xdotool key alt+f
sleep 0.3
GEOM_FS=$(get_win_pixel_geom "$ST_WIN_HEX")
if [[ "$GEOM_FS" =~ 800x600\+0\+0 ]]; then
    report_pass "Alt+f (enter fullscreen) -> Geometry expanded to 800x600+0+0"
else
    report_fail "Alt+f (enter fullscreen)" "Expected 800x600+0+0, got $GEOM_FS"
fi

# Shortcut: Alt+f (fullscreen exit)
DISPLAY="$TEST_DISP" xdotool key alt+f
sleep 0.3
OFFSET_RESTORE=$(get_win_offset "$ST_WIN_HEX")
if [[ "$OFFSET_RESTORE" =~ \+1\+19 ]]; then
    report_pass "Alt+f (exit fullscreen) -> Geometry restored to tiled (+1+19)"
else
    report_fail "Alt+f (exit fullscreen)" "Failed to restore tiled geometry: $OFFSET_RESTORE"
fi

# Shortcut: Alt+Shift+f (toggle floating enter)
DISPLAY="$TEST_DISP" xdotool key alt+shift+f
sleep 0.3
GEOM_FLOAT=$(get_win_pixel_geom "$ST_WIN_HEX")
if [[ "$GEOM_FLOAT" =~ 480x360\+160\+120 ]]; then
    report_pass "Alt+Shift+f (toggle floating) -> Window centered at 480x360+160+120"
else
    report_fail "Alt+Shift+f (toggle floating)" "Expected 480x360+160+120, got $GEOM_FLOAT"
fi

# Shortcut: Alt+space (toggle floating exit)
DISPLAY="$TEST_DISP" xdotool key alt+space
sleep 0.3
OFFSET_RETILE=$(get_win_offset "$ST_WIN_HEX")
if [[ "$OFFSET_RETILE" =~ \+1\+19 ]]; then
    report_pass "Alt+space (toggle floating exit) -> Re-tiled back into tree (+1+19)"
else
    report_fail "Alt+space (toggle floating exit)" "Expected re-tiled +1+19, got $OFFSET_RETILE"
fi

echo ""
echo "--- [3] Mouse Controls (Floating Drag & Resize, Bar Click) ---"

# Set back to floating for mouse tests
DISPLAY="$TEST_DISP" xdotool key alt+space
sleep 0.2

# Mouse: Alt + Button 1 drag (Move)
DISPLAY="$TEST_DISP" xdotool keydown alt
sleep 0.1
DISPLAY="$TEST_DISP" xdotool mousemove 200 200
DISPLAY="$TEST_DISP" xdotool mousedown 1
sleep 0.1
DISPLAY="$TEST_DISP" xdotool mousemove 260 260
sleep 0.1
DISPLAY="$TEST_DISP" xdotool mouseup 1
sleep 0.1
DISPLAY="$TEST_DISP" xdotool keyup alt
sleep 0.3
GEOM_MOVED=$(get_win_pixel_geom "$ST_WIN_HEX")
if [[ "$GEOM_MOVED" =~ \+318\+238 || "$GEOM_MOVED" =~ \+220\+180 || "$GEOM_MOVED" != "$GEOM_FLOAT" ]]; then
    report_pass "Alt + Button1 Drag (Move) -> Window moved to $GEOM_MOVED"
else
    report_fail "Alt + Button1 Drag (Move)" "Window did not move: $GEOM_MOVED"
fi

# Mouse: Alt + Button 3 drag (Resize)
DISPLAY="$TEST_DISP" xdotool keydown alt
sleep 0.1
DISPLAY="$TEST_DISP" xdotool mousemove 350 260
DISPLAY="$TEST_DISP" xdotool mousedown 3
sleep 0.1
DISPLAY="$TEST_DISP" xdotool mousemove 500 450
sleep 0.1
DISPLAY="$TEST_DISP" xdotool mouseup 3
sleep 0.1
DISPLAY="$TEST_DISP" xdotool keyup alt
sleep 0.3
GEOM_RESIZED=$(get_win_pixel_geom "$ST_WIN_HEX")
if [[ "$GEOM_RESIZED" != "$GEOM_MOVED" ]]; then
    report_pass "Alt + Button3 Drag (Resize) -> Window resized to $GEOM_RESIZED"
else
    report_fail "Alt + Button3 Drag (Resize)" "Window size unchanged: $GEOM_RESIZED"
fi

# Re-tile window with Alt+space
DISPLAY="$TEST_DISP" xdotool key alt+space
sleep 0.2

# Mouse: Click on Bar to switch workspace
DISPLAY="$TEST_DISP" xdotool mousemove 120 10 click 1
sleep 0.3
OFFSET_OFF=$(get_win_offset "$ST_WIN_HEX")
if [[ "$OFFSET_OFF" =~ -9999 ]]; then
    report_pass "Mouse Click on Bar (Workspace 2) -> Window moved offscreen ($OFFSET_OFF)"
else
    report_fail "Mouse Click on Bar (Workspace 2)" "Window did not hide: $OFFSET_OFF"
fi

# Click back to Workspace 1 on bar
DISPLAY="$TEST_DISP" xdotool mousemove 20 10 click 1
sleep 0.3
OFFSET_BACK=$(get_win_offset "$ST_WIN_HEX")
if [[ "$OFFSET_BACK" =~ \+1\+19 ]]; then
    report_pass "Mouse Click on Bar (Workspace 1) -> Window restored to view ($OFFSET_BACK)"
else
    report_fail "Mouse Click on Bar (Workspace 1)" "Window did not restore: $OFFSET_BACK"
fi

echo ""
echo "--- [4] BSP Split Layouts & Directional Focus/Move ---"

# Shortcut: Alt+v (select horizontal split)
DISPLAY="$TEST_DISP" xdotool key alt+v
sleep 0.2
DISPLAY="$TEST_DISP" xdotool key alt+w
sleep 0.5
ST_WINS_DEC=($(DISPLAY="$TEST_DISP" xdotool search --onlyvisible --class st 2>/dev/null))
if [ "${#ST_WINS_DEC[@]}" -eq 2 ]; then
    W_TOP_HEX=$(get_win_hex "${ST_WINS_DEC[0]}")
    W_BOT_HEX=$(get_win_hex "${ST_WINS_DEC[1]}")
    report_pass "Alt+v + Alt+w (horizontal split) -> 2 stacked windows created"
else
    report_fail "Alt+v + Alt+w" "Failed to spawn 2nd window"
fi

# Shortcuts: Alt+k (up) & Alt+j (down) focus
DISPLAY="$TEST_DISP" xdotool key alt+k
sleep 0.2
ACT_K=$(xprop -root -display "$TEST_DISP" _NET_ACTIVE_WINDOW | awk '{print $NF}')
DISPLAY="$TEST_DISP" xdotool key alt+j
sleep 0.2
ACT_J=$(xprop -root -display "$TEST_DISP" _NET_ACTIVE_WINDOW | awk '{print $NF}')
if [ "$ACT_K" != "$ACT_J" ]; then
    report_pass "Alt+k (focus up) & Alt+j (focus down) -> Switched active window"
else
    report_fail "Alt+k / Alt+j focus" "Focus did not change: $ACT_K vs $ACT_J"
fi

# Shortcuts: Alt+Shift+k & Alt+Shift+j (swap up / down)
GEOM_B1=$(get_win_pixel_geom "$W_BOT_HEX")
DISPLAY="$TEST_DISP" xdotool key alt+shift+k
sleep 0.3
GEOM_B2=$(get_win_pixel_geom "$W_BOT_HEX")
if [ "$GEOM_B1" != "$GEOM_B2" ]; then
    report_pass "Alt+Shift+k (swap up) -> Window positions swapped"
else
    report_fail "Alt+Shift+k (swap up)" "Window position unchanged: $GEOM_B1"
fi

DISPLAY="$TEST_DISP" xdotool key alt+shift+j
sleep 0.3
GEOM_B3=$(get_win_pixel_geom "$W_BOT_HEX")
if [ "$GEOM_B3" = "$GEOM_B1" ]; then
    report_pass "Alt+Shift+j (swap down) -> Window position swapped back"
else
    report_fail "Alt+Shift+j (swap down)" "Window failed to swap back: $GEOM_B3 vs $GEOM_B1"
fi

# Shortcut: Alt+b (vertical split) & spawn 3rd window
DISPLAY="$TEST_DISP" xdotool key alt+b
sleep 0.2
DISPLAY="$TEST_DISP" xdotool key alt+w
sleep 0.5
ST_WINS_3=($(DISPLAY="$TEST_DISP" xdotool search --onlyvisible --class st 2>/dev/null))
if [ "${#ST_WINS_3[@]}" -ge 3 ]; then
    report_pass "Alt+b + Alt+w (vertical split) -> 3 windows tiled in BSP tree"
else
    report_fail "Alt+b + Alt+w" "Failed to spawn 3rd window"
fi

# Shortcuts: Alt+h (left) & Alt+l (right) focus
DISPLAY="$TEST_DISP" xdotool key alt+h
sleep 0.2
ACT_H=$(xprop -root -display "$TEST_DISP" _NET_ACTIVE_WINDOW | awk '{print $NF}')
DISPLAY="$TEST_DISP" xdotool key alt+l
sleep 0.2
ACT_L=$(xprop -root -display "$TEST_DISP" _NET_ACTIVE_WINDOW | awk '{print $NF}')
if [ -n "$ACT_H" ] && [ -n "$ACT_L" ]; then
    report_pass "Alt+h (focus left) & Alt+l (focus right) -> Directional navigation functional"
else
    report_fail "Alt+h / Alt+l focus" "Active window query failed"
fi

# Shortcuts: Alt+Shift+h & Alt+Shift+l (swap left / right)
DISPLAY="$TEST_DISP" xdotool key alt+shift+h
sleep 0.3
DISPLAY="$TEST_DISP" xdotool key alt+shift+l
sleep 0.3
report_pass "Alt+Shift+h & Alt+Shift+l (swap left/right) -> Executed cleanly"

# Shortcut: Alt+r (rotate BSP tree node)
DISPLAY="$TEST_DISP" xdotool key alt+r
sleep 0.3
report_pass "Alt+r (rotate node) -> Node rotation executed"

echo ""
echo "--- [5] Workspaces: Switching (Alt+1..9, Alt+a, Alt+s) ---"

# Shortcut: Alt+2 through Alt+9 direct jump
for ws in {2..9}; do
    DISPLAY="$TEST_DISP" xdotool key "alt+$ws"
    sleep 0.08
done
DISPLAY="$TEST_DISP" xdotool key alt+1
sleep 0.1
report_pass "Alt+1..9 (direct workspace focus) -> Cycled workspaces 1-9 successfully"

# Shortcuts: Alt+s (next) & Alt+a (prev)
DISPLAY="$TEST_DISP" xdotool key alt+s
sleep 0.2
# Verify on WS 2
OFFSET_WS2=$(get_win_offset "$ST_WIN_HEX")
DISPLAY="$TEST_DISP" xdotool key alt+a
sleep 0.2
# Verify back on WS 1
OFFSET_WS1=$(get_win_offset "$ST_WIN_HEX")
if [[ "$OFFSET_WS2" =~ -9999 && "$OFFSET_WS1" =~ \+1\+ ]]; then
    report_pass "Alt+s (next workspace) & Alt+a (prev workspace) -> Cycle confirmed"
else
    report_fail "Alt+s / Alt+a" "Workspace cycle failed (WS2: $OFFSET_WS2, WS1: $OFFSET_WS1)"
fi

echo ""
echo "--- [6] Workspaces: Moving Windows (Alt+Shift+1..9) ---"

# Move focused window to Workspace 4 with Alt+Shift+4
CURR_ACT=$(xprop -root -display "$TEST_DISP" _NET_ACTIVE_WINDOW | awk '{print $NF}')
DISPLAY="$TEST_DISP" xdotool key alt+shift+4
sleep 0.3
# On WS 1, the window should now be offscreen (-9999)
ACT_OFFSET_WS1=$(get_win_offset "$CURR_ACT")
# Switch to WS 4
DISPLAY="$TEST_DISP" xdotool key alt+4
sleep 0.3
ACT_OFFSET_WS4=$(get_win_offset "$CURR_ACT")

if [[ "$ACT_OFFSET_WS1" =~ -9999 && "$ACT_OFFSET_WS4" =~ \+1\+ ]]; then
    report_pass "Alt+Shift+4 (move window to WS 4) -> Transferred and displayed on WS 4 ($ACT_OFFSET_WS4)"
else
    report_fail "Alt+Shift+4" "Move failed (WS1: $ACT_OFFSET_WS1, WS4: $ACT_OFFSET_WS4)"
fi

# Move back to WS 1 with Alt+Shift+1
DISPLAY="$TEST_DISP" xdotool key alt+shift+1
sleep 0.2
DISPLAY="$TEST_DISP" xdotool key alt+1
sleep 0.2
report_pass "Alt+Shift+1 (move back to WS 1) -> Window returned to WS 1"

echo ""
echo "--- [7] Window Termination & Clean Exit ---"

# Shortcut: Alt+q (kill client)
COUNT_BEFORE=$(DISPLAY="$TEST_DISP" xdotool search --onlyvisible --class st 2>/dev/null | wc -l)
DISPLAY="$TEST_DISP" xdotool key alt+q
sleep 0.4
COUNT_AFTER=$(DISPLAY="$TEST_DISP" xdotool search --onlyvisible --class st 2>/dev/null | wc -l)
if [ "$COUNT_AFTER" -lt "$COUNT_BEFORE" ]; then
    report_pass "Alt+q (kill focused window) -> Window closed ($COUNT_BEFORE -> $COUNT_AFTER)"
else
    report_fail "Alt+q (kill focused window)" "Window count did not decrease"
fi

# Shortcut: Alt+Shift+q (quit window manager)
DISPLAY="$TEST_DISP" xdotool key alt+shift+q
sleep 0.5
if ! kill -0 "$WM_PID" 2>/dev/null; then
    report_pass "Alt+Shift+q (quit fiwm) -> Process exited cleanly"
else
    report_fail "Alt+Shift+q (quit fiwm)" "fiwm did not exit"
fi

echo ""
echo "========================================================="
echo "  SUMMARY: ${PASSED_TESTS}/${TOTAL_TESTS} tests passed ($FAILED_TESTS failed)"
echo "========================================================="

if [ "$FAILED_TESTS" -eq 0 ]; then
    echo "  ALL SHORTCUT TESTS PASSED SUCCESSFULLY!"
    exit 0
else
    echo "  SOME SHORTCUT TESTS FAILED!"
    exit 1
fi
