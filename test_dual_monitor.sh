#!/usr/bin/env bash
# test_dual_monitor.sh - Automated dual monitor (Xinerama) verification for fiwm
set -euo pipefail

TEST_DISP=":97"
XEPHYR_LOG="/tmp/xephyr_disp97.log"
TEST_LOG="/tmp/fiwm_dual_test.log"
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

get_win_hex() {
    local dec_id="$1"
    printf "0x%x" "$dec_id"
}

get_win_offset() {
    local hex_id="$1"
    xwininfo -root -tree -display "$TEST_DISP" 2>/dev/null | grep "$hex_id" | head -n 1 | awk '{print $NF}'
}

cleanup() {
    if [ -n "${WM_PID:-}" ] && kill -0 "$WM_PID" 2>/dev/null; then
        kill "$WM_PID" 2>/dev/null || true
    fi
    if [ -n "${XE_PID:-}" ] && kill -0 "$XE_PID" 2>/dev/null; then
        kill "$XE_PID" 2>/dev/null || true
    fi
    pkill -f "st.*$TEST_DISP" 2>/dev/null || true
    rm -f "/tmp/.X11-unix/X${TEST_DISP#:}"
    rm -f "$XEPHYR_LOG" "$TEST_LOG"
}
trap cleanup EXIT INT TERM

echo "========================================================="
echo "       fiwm Dual Monitor (Xinerama) Test Suite           "
echo "========================================================="

# 1. Launch Xephyr with 2 screens: Head 0 (800x600 @ 0,0) and Head 1 (1024x768 @ 800,0)
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

# 2. Start fiwm
DISPLAY="$TEST_DISP" ./fiwm >"$TEST_LOG" 2>&1 &
WM_PID=$!
sleep 0.6

if ! kill -0 "$WM_PID" 2>/dev/null; then
    echo "[!] fiwm crashed on dual-head initialization!"
    cat "$TEST_LOG"
    exit 1
fi
report_pass "fiwm started cleanly on dual-head display"

# 3. Test initial workspace (Workspace 1 on Monitor 1 @ X >= 800)
DISPLAY="$TEST_DISP" xdotool key alt+w
sleep 0.6

W1_DEC=$(DISPLAY="$TEST_DISP" xdotool search --onlyvisible --class st 2>/dev/null | head -n 1 || true)
if [ -n "$W1_DEC" ]; then
    W1_HEX=$(get_win_hex "$W1_DEC")
    W1_OFFSET=$(get_win_offset "$W1_HEX")
    # Offset format: +X+Y, e.g. +801+19
    if [[ "$W1_OFFSET" =~ \+([0-9]+)\+([0-9]+) ]] && [ "${BASH_REMATCH[1]}" -ge 800 ]; then
        report_pass "Workspace 1 mapped to Monitor 1 (Window at $W1_OFFSET, X >= 800)"
    else
        report_fail "Workspace 1 monitor mapping" "Expected window on Monitor 1 (X >= 800), got $W1_OFFSET"
    fi
else
    report_fail "Spawn window" "Window on Workspace 1 not detected"
fi

# 4. Switch to Workspace 9 (Pinned to Monitor 0 @ X < 800)
DISPLAY="$TEST_DISP" xdotool key alt+9
sleep 0.4
DISPLAY="$TEST_DISP" xdotool key alt+w
sleep 0.6

W9_DEC=$(DISPLAY="$TEST_DISP" xdotool search --onlyvisible --class st 2>/dev/null | tail -n 1 || true)
if [ -n "$W9_DEC" ]; then
    W9_HEX=$(get_win_hex "$W9_DEC")
    W9_OFFSET=$(get_win_offset "$W9_HEX")
    if [[ "$W9_OFFSET" =~ \+([0-9]+)\+([0-9]+) ]] && [ "${BASH_REMATCH[1]}" -lt 800 ]; then
        report_pass "Workspace 9 mapped to Monitor 0 (Window at $W9_OFFSET, X < 800)"
    else
        report_fail "Workspace 9 monitor mapping" "Expected window on Monitor 0 (X < 800), got $W9_OFFSET"
    fi
else
    report_fail "Spawn window" "Window on Workspace 9 not detected"
fi

# 5. Cross-Monitor Move: Send focused window on Workspace 9 to Workspace 1 (Monitor 0 -> Monitor 1)
DISPLAY="$TEST_DISP" xdotool key alt+shift+1
sleep 0.4
DISPLAY="$TEST_DISP" xdotool key alt+1
sleep 0.4

W_TRANS_OFFSET=$(get_win_offset "$W9_HEX")
if [[ "$W_TRANS_OFFSET" =~ \+([0-9]+)\+([0-9]+) ]] && [ "${BASH_REMATCH[1]}" -ge 800 ]; then
    report_pass "Cross-monitor transfer (Alt+Shift+1): Window moved to Monitor 1 ($W_TRANS_OFFSET)"
else
    report_fail "Cross-monitor transfer" "Expected X >= 800, got $W_TRANS_OFFSET"
fi

# 6. Clean exit
DISPLAY="$TEST_DISP" xdotool key alt+shift+q
sleep 0.5
if ! kill -0 "$WM_PID" 2>/dev/null; then
    report_pass "Alt+Shift+q: Clean shutdown on dual-head setup"
else
    report_fail "Shutdown" "fiwm did not exit cleanly"
fi

echo ""
echo "========================================================="
echo "  DUAL MONITOR SUMMARY: ${PASSED_TESTS}/${TOTAL_TESTS} tests passed ($FAILED_TESTS failed)"
echo "========================================================="

if [ "$FAILED_TESTS" -eq 0 ]; then
    echo "  ALL DUAL MONITOR TESTS PASSED SUCCESSFULLY!"
    exit 0
else
    echo "  SOME DUAL MONITOR TESTS FAILED!"
    exit 1
fi
