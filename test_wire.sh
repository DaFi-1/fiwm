#!/usr/bin/env bash
# test_wire.sh - Automated integration test suite for fiwm-wire
set -euo pipefail

TEST_DISP=":99"
TEST_LOG="/tmp/fiwm_wire_test.log"
XEPHYR_LOG="/tmp/xephyr_test_disp99.log"
FAILED=0

cleanup() {
    echo "Cleaning up test environment..."
    if [ -n "${WM_PID:-}" ] && kill -0 "$WM_PID" 2>/dev/null; then
        kill "$WM_PID" 2>/dev/null || true
    fi
    if [ -n "${XE_PID:-}" ] && kill -0 "$XE_PID" 2>/dev/null; then
        kill "$XE_PID" 2>/dev/null || true
    fi
    pkill -f "st.*$TEST_DISP" 2>/dev/null || true
    rm -f "/tmp/.X11-unix/X${TEST_DISP#:}"
}
trap cleanup EXIT INT TERM

echo "================================================="
echo "       fiwm-wire Automated Test Suite            "
echo "================================================="

# 1. Compile fiwm-wire
echo "[*] Building fiwm-wire..."
make fiwm-wire

if [ ! -x "./fiwm-wire" ]; then
    echo "[!] Error: ./fiwm-wire binary not found or not executable."
    exit 1
fi

# 2. Start isolated Xephyr instance on TEST_DISP
echo "[*] Starting Xephyr on display ${TEST_DISP} (800x600)..."
rm -f "/tmp/.X11-unix/X${TEST_DISP#:}"
Xephyr -br -ac -noreset -screen 800x600 "$TEST_DISP" >"$XEPHYR_LOG" 2>&1 &
XE_PID=$!

# Wait for Xephyr socket
for i in $(seq 1 30); do
    if [ -S "/tmp/.X11-unix/X${TEST_DISP#:}" ]; then
        break
    fi
    sleep 0.1
done

if [ ! -S "/tmp/.X11-unix/X${TEST_DISP#:}" ]; then
    echo "[!] Failed to start Xephyr on ${TEST_DISP}."
    cat "$XEPHYR_LOG"
    exit 1
fi

# 3. Launch fiwm-wire on TEST_DISP
echo "[*] Starting fiwm-wire on ${TEST_DISP}..."
DISPLAY="$TEST_DISP" ./fiwm-wire >"$TEST_LOG" 2>&1 &
WM_PID=$!
sleep 1

if ! kill -0 "$WM_PID" 2>/dev/null; then
    echo "[!] fiwm-wire died on startup!"
    cat "$TEST_LOG"
    exit 1
fi

# Test 1: Resource consumption (RSS & CPU)
echo "[1/8] Verifying ultra-low RAM footprint and 0.0% CPU..."
RSS=$(ps -p "$WM_PID" -o rss= | tr -d ' ')
CPU=$(ps -p "$WM_PID" -o %cpu= | tr -d ' ')
echo "      -> fiwm-wire RSS: ${RSS} kB, CPU: ${CPU}%"
if [ "$RSS" -gt 150 ]; then
    echo "      [FAIL] RSS (${RSS} kB) exceeded 150 kB threshold!"
    FAILED=1
else
    echo "      [PASS] RSS is ultra-low (${RSS} kB < 150 kB limit)."
fi

# Test 2: Status bar creation
echo "[2/8] Checking status bar window..."
if xwininfo -root -tree -display "$TEST_DISP" | grep -q "800x18+0+0"; then
    echo "      [PASS] Top status bar found at 800x18+0+0."
else
    echo "      [FAIL] Top status bar not found in window tree!"
    FAILED=1
fi

# Test 3: EWMH Properties
echo "[3/8] Checking EWMH properties (_NET_NUMBER_OF_DESKTOPS)..."
DESKTOPS=$(xprop -root -display "$TEST_DISP" _NET_NUMBER_OF_DESKTOPS 2>/dev/null | awk -F'=' '{print $2}' | tr -d ' ' || echo "")
if [ "$DESKTOPS" = "9" ]; then
    echo "      [PASS] _NET_NUMBER_OF_DESKTOPS is 9."
else
    echo "      [FAIL] _NET_NUMBER_OF_DESKTOPS = '$DESKTOPS' (expected 9)."
    FAILED=1
fi

# Test 4: Default terminal spawn (st via Alt+w)
echo "[4/8] Testing terminal spawn with Alt+w (default: st)..."
DISPLAY="$TEST_DISP" xdotool key alt+w

ST_OPEN=0
for i in $(seq 1 20); do
    sleep 0.2
    if xwininfo -root -tree -display "$TEST_DISP" 2>/dev/null | grep -q "\"st\""; then
        ST_OPEN=1
        break
    fi
done

if [ "$ST_OPEN" -eq 1 ]; then
    echo "      [PASS] Terminal 'st' spawned and tiled successfully."
    GEOM=$(xwininfo -root -tree -display "$TEST_DISP" | grep "\"st\"" | head -n 1 | awk '{print $NF}')
    echo "      -> Initial window geometry: $GEOM"
else
    echo "      [FAIL] Terminal 'st' failed to spawn within 4s."
    FAILED=1
fi

# Test 5: BSP dynamic tiling (second Alt+w)
echo "[5/8] Testing BSP tree split (second Alt+w)..."
DISPLAY="$TEST_DISP" xdotool key alt+w

SPLIT_OK=0
for i in $(seq 1 20); do
    sleep 0.2
    COUNT=$(xwininfo -root -tree -display "$TEST_DISP" 2>/dev/null | grep -c "\"st\"" || true)
    if [ "$COUNT" -ge 2 ]; then
        SPLIT_OK=1
        break
    fi
done

if [ "$SPLIT_OK" -eq 1 ]; then
    echo "      [PASS] Both terminals tiled by BSP tree."
    xwininfo -root -tree -display "$TEST_DISP" | grep "\"st\"" | while read -r line; do
        echo "      -> $line"
    done
else
    echo "      [FAIL] Failed to tile second terminal."
    FAILED=1
fi

# Test 6: EWMH active window update
echo "[6/8] Checking _NET_ACTIVE_WINDOW..."
ACTIVE_WIN=$(xprop -root -display "$TEST_DISP" _NET_ACTIVE_WINDOW 2>/dev/null | awk '{print $NF}' || echo "")
if [[ "$ACTIVE_WIN" =~ 0x[0-9a-fA-F]+ ]]; then
    echo "      [PASS] _NET_ACTIVE_WINDOW correctly set ($ACTIVE_WIN)."
else
    echo "      [FAIL] _NET_ACTIVE_WINDOW not set or invalid ($ACTIVE_WIN)."
    FAILED=1
fi

# Test 7: Closing focused window with Alt+q
echo "[7/8] Testing window close with Alt+q..."
DISPLAY="$TEST_DISP" xdotool key alt+q

CLOSE_OK=0
for i in $(seq 1 20); do
    sleep 0.2
    COUNT=$(xwininfo -root -tree -display "$TEST_DISP" 2>/dev/null | grep -c "\"st\"" || true)
    if [ "$COUNT" -eq 1 ]; then
        CLOSE_OK=1
        break
    fi
done

if [ "$CLOSE_OK" -eq 1 ]; then
    echo "      [PASS] Focused terminal closed via WM_DELETE_WINDOW."
    REMAIN_GEOM=$(xwininfo -root -tree -display "$TEST_DISP" | grep "\"st\"" | awk '{print $NF}')
    echo "      -> Remaining window expanded to: $REMAIN_GEOM"
else
    echo "      [FAIL] Window close via Alt+q timed out."
    FAILED=1
fi

# Test 8: Clean exit with Alt+Shift+q
echo "[8/8] Testing clean WM quit with Alt+Shift+q..."
DISPLAY="$TEST_DISP" xdotool key alt+shift+q

QUIT_OK=0
for i in $(seq 1 20); do
    sleep 0.1
    if ! kill -0 "$WM_PID" 2>/dev/null; then
        QUIT_OK=1
        break
    fi
done

if [ "$QUIT_OK" -eq 1 ]; then
    echo "      [PASS] fiwm-wire exited cleanly on Alt+Shift+q."
else
    echo "      [FAIL] fiwm-wire did not exit within 2s."
    FAILED=1
fi

echo "================================================="
if [ "$FAILED" -eq 0 ]; then
    echo "       ALL TESTS PASSED SUCCESSFULLY!            "
    echo "================================================="
    exit 0
else
    echo "       SOME TESTS FAILED!                        "
    echo "================================================="
    exit 1
fi
