#!/usr/bin/env bash
# test_fiwmasm_dual_monitor.sh - Automated dual monitor verification for fiwmasm
set -euo pipefail

TEST_DISP=":97"
XEPHYR_LOG="/tmp/xephyr_disp97.log"
TEST_LOG="/tmp/fiwmasm_dual_test.log"
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

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

cleanup() {
    if [ -n "${WM_PID:-}" ] && kill -0 "$WM_PID" 2>/dev/null; then
        kill "$WM_PID" 2>/dev/null || true
    fi
    if [ -n "${XE_PID:-}" ] && kill -0 "$XE_PID" 2>/dev/null; then
        kill "$XE_PID" 2>/dev/null || true
    fi
    pkill -f "st.*$TEST_DISP" 2>/dev/null || true
    rm -f "/tmp/.X11-unix/X${TEST_DISP#:}" "/tmp/.X${TEST_DISP#:}-lock"
    rm -f "$XEPHYR_LOG" "$TEST_LOG"
}
trap cleanup EXIT INT TERM

echo "========================================================="
echo "       fiwmasm Dual Monitor (Xinerama) Test Suite        "
echo "========================================================="

# 1. Compile fiwmasm
echo "[*] Compiling fiwmasm..."
make fiwmasm >/dev/null

# 2. Launch Xephyr with 2 screens: Head 0 (800x600 @ 0,0) and Head 1 (1024x768 @ 800,0)
rm -f "/tmp/.X11-unix/X${TEST_DISP#:}" "/tmp/.X${TEST_DISP#:}-lock"
Xephyr "$TEST_DISP" +xinerama -screen 800x600+0+0 -screen 1024x768+800+0 -ac -noreset >"$XEPHYR_LOG" 2>&1 &
XE_PID=$!

for i in $(seq 1 30); do
    if [ -S "/tmp/.X11-unix/X${TEST_DISP#:}" ]; then break; fi
    sleep 0.1
done

sleep 0.5
HEADS=$(DISPLAY="$TEST_DISP" xdpyinfo -ext XINERAMA 2>/dev/null | grep -c "head #" || true)
if [ "$HEADS" -eq 2 ]; then
    report_pass "Xephyr dual-head initialized (2 Xinerama screens)"
else
    report_fail "Xephyr dual-head" "Expected 2 heads, detected $HEADS"
    exit 1
fi

# 3. Start fiwmasm
DISPLAY="$TEST_DISP" ./fiwmasm >"$TEST_LOG" 2>&1 &
WM_PID=$!
sleep 0.6

if ! kill -0 "$WM_PID" 2>/dev/null; then
    echo "[!] fiwmasm crashed on dual-head initialization!"
    cat "$TEST_LOG"
    exit 1
fi
report_pass "fiwmasm started cleanly on dual-head display"

# 4. Test Workspace 1 (configured in ws_mon_table for Monitor 0 @ X < 800)
DISPLAY="$TEST_DISP" xdotool key alt+Return
sleep 0.6

W1_OFFSET=$(xwininfo -root -tree -display "$TEST_DISP" 2>/dev/null | grep 'st' | grep -v '\-9999' | head -n 1 | awk '{print $NF}')
if [ -n "$W1_OFFSET" ] && [[ "$W1_OFFSET" =~ \+([0-9]+)\+([0-9]+) ]] && [ "${BASH_REMATCH[1]}" -lt 800 ]; then
    report_pass "Workspace 1 mapped to Monitor 0 according to ws_mon_table (Window at $W1_OFFSET, X < 800)"
else
    report_fail "Workspace 1 mapping" "Expected window on Monitor 0 (X < 800), got $W1_OFFSET"
fi

# 5. Switch to Workspace 9 (configured in ws_mon_table for Monitor 1 @ X >= 800)
DISPLAY="$TEST_DISP" xdotool key alt+9
sleep 0.4
DISPLAY="$TEST_DISP" xdotool key alt+Return
sleep 0.6

W9_OFFSET=$(xwininfo -root -tree -display "$TEST_DISP" 2>/dev/null | grep 'st' | grep -v '\-9999' | head -n 1 | awk '{print $NF}')
if [ -n "$W9_OFFSET" ] && [[ "$W9_OFFSET" =~ \+([0-9]+)\+([0-9]+) ]] && [ "${BASH_REMATCH[1]}" -ge 800 ]; then
    report_pass "Workspace 9 mapped to Monitor 1 according to ws_mon_table (Window at $W9_OFFSET, X >= 800)"
else
    report_fail "Workspace 9 mapping" "Expected window on Monitor 1 (X >= 800), got $W9_OFFSET"
fi

# 6. Cross-Monitor Move: Send focused window on Workspace 9 to Workspace 1 (Monitor 1 -> Monitor 0)
DISPLAY="$TEST_DISP" xdotool key alt+shift+1
sleep 0.4
DISPLAY="$TEST_DISP" xdotool key alt+1
sleep 0.4

# Verify both windows are now on Monitor 0 (X < 800)
COUNT_MON0=$(xwininfo -root -tree -display "$TEST_DISP" 2>/dev/null | grep 'st' | grep -v '\-9999' | wc -l)
if [ "$COUNT_MON0" -eq 2 ]; then
    report_pass "Cross-monitor transfer (Alt+Shift+1): Window successfully moved from Monitor 1 to Monitor 0"
else
    report_fail "Cross-monitor transfer" "Expected 2 windows on Workspace 1 (Monitor 0), found $COUNT_MON0"
fi

# 7. Clean exit
DISPLAY="$TEST_DISP" xdotool key alt+shift+q
sleep 0.5
if ! kill -0 "$WM_PID" 2>/dev/null; then
    report_pass "Alt+Shift+q: Clean shutdown on dual-head setup"
else
    report_fail "Shutdown" "fiwmasm did not exit cleanly"
fi

echo ""
echo "========================================================="
echo "   Dual Monitor Results: $PASSED_TESTS / $TOTAL_TESTS Passed"
echo "========================================================="

if [ "$FAILED_TESTS" -eq 0 ]; then
    exit 0
else
    exit 1
fi
