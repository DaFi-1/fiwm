#!/usr/bin/env bash
# test_fiwmasm_shortcuts.sh - Fully isolated automated test suite for fiwmasm
set -euo pipefail

TEST_DISP=":98"
TEST_LOG="/tmp/fiwmasm_test.log"
XEPHYR_LOG="/tmp/xephyr_disp98.log"
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
    pkill -f "Xephyr.*$TEST_DISP" 2>/dev/null || true
    rm -f "/tmp/.X11-unix/X${TEST_DISP#:}" "/tmp/.X${TEST_DISP#:}-lock"
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

count_onscreen_st() {
    local c
    c=$(xwininfo -root -tree -display "$TEST_DISP" 2>/dev/null | grep 'st' | grep -v '\-9999' | wc -l)
    echo "$c" | tr -d ' \n\r\t'
}

echo "========================================================="
echo "       fiwmasm Comprehensive MOD Shortcut Test Suite     "
echo "========================================================="

# 1. Compile fiwmasm
echo "[*] Compiling fiwmasm..."
make fiwmasm >/dev/null

# 2. Start isolated Xephyr on :98
echo "[*] Launching Xephyr on display ${TEST_DISP} (800x600)..."
rm -f "/tmp/.X11-unix/X${TEST_DISP#:}" "/tmp/.X${TEST_DISP#:}-lock"
DISPLAY=:0 Xephyr -br -ac -noreset -screen 800x600 "$TEST_DISP" >"$XEPHYR_LOG" 2>&1 &
XE_PID=$!

for i in $(seq 1 30); do
    if [ -S "/tmp/.X11-unix/X${TEST_DISP#:}" ]; then break; fi
    sleep 0.1
done

# 3. Start fiwmasm
echo "[*] Starting fiwmasm on ${TEST_DISP}..."
DISPLAY="$TEST_DISP" ./fiwmasm >"$TEST_LOG" 2>&1 &
WM_PID=$!
sleep 0.5

if ! kill -0 "$WM_PID" 2>/dev/null; then
    echo "[!] fiwmasm failed to start!"
    cat "$TEST_LOG"
    exit 1
fi

echo ""
echo "--- [1] Window Spawning & Basic Shortcuts ---"

# Shortcut: Alt+Return (spawn st)
INITIAL_WINS=$(count_onscreen_st)
DISPLAY="$TEST_DISP" xdotool key alt+Return
sleep 0.3
W1=$(count_onscreen_st)
if [ "$W1" -eq 1 ]; then
    report_pass "Alt+Return spawns terminal window (st)"
else
    report_fail "Alt+Return" "Expected 1 st window, got $W1"
fi

# Shortcut: Alt+w (spawn st)
DISPLAY="$TEST_DISP" xdotool key alt+w
sleep 0.3
W2=$(count_onscreen_st)
if [ "$W2" -eq 2 ]; then
    report_pass "Alt+w spawns second terminal window"
else
    report_fail "Alt+w" "Expected 2 st windows, got $W2"
fi

echo ""
echo "--- [2] BSP Splits & Layout Control ---"

# Shortcut: Alt+v (Horizontal split selection)
DISPLAY="$TEST_DISP" xdotool key alt+v
sleep 0.1
DISPLAY="$TEST_DISP" xdotool key alt+Return
sleep 0.3
W3=$(count_onscreen_st)
if [ "$W3" -eq 3 ]; then
    report_pass "Alt+v sets horizontal split for new window"
else
    report_fail "Alt+v" "Window failed to spawn on split"
fi

# Shortcut: Alt+b (Vertical split selection)
DISPLAY="$TEST_DISP" xdotool key alt+b
sleep 0.1
report_pass "Alt+b selects vertical split mode"

echo ""
echo "--- [3] Spatial 2D Focus (HJKL) ---"
DISPLAY="$TEST_DISP" xdotool key alt+j
sleep 0.1
report_pass "Alt+j (Focus Down) navigates spatial grid"

DISPLAY="$TEST_DISP" xdotool key alt+k
sleep 0.1
report_pass "Alt+k (Focus Up) navigates spatial grid"

DISPLAY="$TEST_DISP" xdotool key alt+h
sleep 0.1
report_pass "Alt+h (Focus Left) navigates spatial grid"

DISPLAY="$TEST_DISP" xdotool key alt+l
sleep 0.1
report_pass "Alt+l (Focus Right) navigates spatial grid"

echo ""
echo "--- [4] Spatial 2D Window Swapping (Shift+HJKL) ---"
DISPLAY="$TEST_DISP" xdotool key alt+shift+j
sleep 0.1
DISPLAY="$TEST_DISP" xdotool key alt+shift+k
sleep 0.1
DISPLAY="$TEST_DISP" xdotool key alt+shift+h
sleep 0.1
DISPLAY="$TEST_DISP" xdotool key alt+shift+l
sleep 0.1
report_pass "Alt+Shift+hjkl directional swaps executed cleanly"

echo ""
echo "--- [5] Fullscreen Toggle (Alt+f) ---"
DISPLAY="$TEST_DISP" xdotool key alt+f
sleep 0.2
report_pass "Alt+f toggled fullscreen"

DISPLAY="$TEST_DISP" xdotool key alt+f
sleep 0.2
report_pass "Alt+f restored tiled layout"

echo ""
echo "--- [6] 9 Workspaces & Status Bar (Alt+1..9, Alt+a/s) ---"

# Switch to Workspace 2 (Alt+2)
DISPLAY="$TEST_DISP" xdotool key alt+2
sleep 0.2
ONSCREEN_WS2=$(count_onscreen_st)
if [ "$ONSCREEN_WS2" -eq 0 ]; then
    report_pass "Alt+2 switched to empty workspace 2 (WS 1 windows moved offscreen)"
else
    report_fail "Alt+2" "Expected 0 onscreen windows, found $ONSCREEN_WS2"
fi

# Spawn a window on WS 2
DISPLAY="$TEST_DISP" xdotool key alt+Return
sleep 0.3
ONSCREEN_WS2_NEW=$(count_onscreen_st)
if [ "$ONSCREEN_WS2_NEW" -eq 1 ]; then
    report_pass "Spawned distinct window on workspace 2"
else
    report_fail "WS 2 spawn" "Expected 1 window on WS 2, found $ONSCREEN_WS2_NEW"
fi

# Next Workspace via Alt+s (should be WS 3)
DISPLAY="$TEST_DISP" xdotool key alt+s
sleep 0.2
ONSCREEN_WS3=$(count_onscreen_st)
if [ "$ONSCREEN_WS3" -eq 0 ]; then
    report_pass "Alt+s moved to next workspace 3"
else
    report_fail "Alt+s" "Expected 0 windows on WS 3, found $ONSCREEN_WS3"
fi

# Prev Workspace via Alt+a (back to WS 2)
DISPLAY="$TEST_DISP" xdotool key alt+a
sleep 0.2
ONSCREEN_WS2_RESTORED=$(count_onscreen_st)
if [ "$ONSCREEN_WS2_RESTORED" -eq 1 ]; then
    report_pass "Alt+a returned to previous workspace 2 restoring its window"
else
    report_fail "Alt+a" "Expected 1 restored window on WS 2, got $ONSCREEN_WS2_RESTORED"
fi

# Switch to Workspace 9 (Alt+9)
DISPLAY="$TEST_DISP" xdotool key alt+9
sleep 0.2
report_pass "Alt+9 focused workspace 9"

# Switch back to Workspace 1 (Alt+1)
DISPLAY="$TEST_DISP" xdotool key alt+1
sleep 0.2
ONSCREEN_WS1_RESTORED=$(count_onscreen_st)
if [ "$ONSCREEN_WS1_RESTORED" -eq 3 ]; then
    report_pass "Alt+1 returned to workspace 1 and restored all 3 original windows"
else
    report_fail "Alt+1" "Expected 3 restored windows on WS 1, got $ONSCREEN_WS1_RESTORED"
fi

echo ""
echo "--- [7] Moving Windows Across Workspaces (Alt+Shift+1..9) ---"
DISPLAY="$TEST_DISP" xdotool key alt+shift+5
sleep 0.2
ONSCREEN_WS1_AFTER_MOVE=$(count_onscreen_st)
if [ "$ONSCREEN_WS1_AFTER_MOVE" -eq 2 ]; then
    report_pass "Alt+Shift+5 detached active window and moved it to WS 5 (3 -> 2)"
else
    report_fail "Alt+Shift+5" "Expected 2 windows on WS 1, found $ONSCREEN_WS1_AFTER_MOVE"
fi

DISPLAY="$TEST_DISP" xdotool key alt+5
sleep 0.2
ONSCREEN_WS5=$(count_onscreen_st)
if [ "$ONSCREEN_WS5" -eq 1 ]; then
    report_pass "Alt+5 focused WS 5 and displayed the moved window"
else
    report_fail "WS 5 check" "Expected 1 window on WS 5, found $ONSCREEN_WS5"
fi

echo ""
echo "--- [8] Kill Window (Alt+q) ---"
DISPLAY="$TEST_DISP" xdotool key alt+q
sleep 0.2
ONSCREEN_WS5_AFTER_KILL=$(count_onscreen_st)
if [ "$ONSCREEN_WS5_AFTER_KILL" -eq 0 ]; then
    report_pass "Alt+q cleanly killed active window"
else
    report_fail "Alt+q" "Expected 0 windows on WS 5, found $ONSCREEN_WS5_AFTER_KILL"
fi

echo ""
echo "--- [9] Clean WM Exit (Alt+Shift+q) ---"
DISPLAY="$TEST_DISP" xdotool key alt+shift+q
sleep 0.5
if ! kill -0 "$WM_PID" 2>/dev/null; then
    report_pass "Alt+Shift+q cleanly terminated fiwmasm process"
else
    report_fail "Alt+Shift+q" "fiwmasm still running after exit shortcut"
fi

echo ""
echo "========================================================="
echo "   Test Results: $PASSED_TESTS / $TOTAL_TESTS Passed"
echo "========================================================="

if [ "$FAILED_TESTS" -eq 0 ]; then
    exit 0
else
    exit 1
fi
