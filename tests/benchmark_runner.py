#!/usr/bin/env python3
import subprocess
import time
import os
import re
import json

TEST_DISP = ":98"
DISP_NUM = TEST_DISP.lstrip(":")
LOG_DIR = "/tmp/fiwm_bench"
os.makedirs(LOG_DIR, exist_ok=True)

BINARIES = {
    "C_Version (fiwm)": "./fiwm",
    "Baseline_ASM (Pre-30 Opts)": "/tmp/fiwmasm_baseline",
    "Running_Version (Em Uso - PID 740)": "/tmp/fiwmasm_running",
    "Current_Version (30 Opts Concluídas)": "./fiwmasm"
}

def cleanup():
    subprocess.run(["pkill", "-9", "-f", f"Xephyr.*{TEST_DISP}"], stderr=subprocess.DEVNULL)
    subprocess.run(["pkill", "-9", "-f", f"st.*{TEST_DISP}"], stderr=subprocess.DEVNULL)
    subprocess.run(["pkill", "-9", "-f", "headless_parent.py"], stderr=subprocess.DEVNULL)
    for f in [f"/tmp/.X11-unix/X{DISP_NUM}", f"/tmp/.X{DISP_NUM}-lock"]:
        if os.path.exists(f):
            try:
                os.remove(f)
            except:
                pass

def run_benchmark_for_binary(name, bin_path):
    cleanup()
    time.sleep(0.2)
    
    # 1. Start headless parent
    p_parent = subprocess.Popen(["python3", "/home/a/fiwm/tests/headless_parent.py", "800", "600"],
                                stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    time.sleep(0.2)
    line = p_parent.stdout.readline().decode().strip()
    parent_win = line if line else ""
    
    # 2. Start Xephyr
    xephyr_cmd = ["Xephyr", "-br", "-ac", "-noreset", "-screen", "800x600"]
    if parent_win:
        xephyr_cmd.extend(["-parent", parent_win])
    xephyr_cmd.append(TEST_DISP)
    
    env = os.environ.copy()
    env["DISPLAY"] = ":0"
    p_xephyr = subprocess.Popen(xephyr_cmd, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    
    sock_path = f"/tmp/.X11-unix/X{DISP_NUM}"
    for _ in range(50):
        if os.path.exists(sock_path):
            break
        time.sleep(0.05)
    
    # 3. Benchmark startup & run with perf stat
    perf_out = f"{LOG_DIR}/perf_{name.split()[0]}.txt"
    wm_env = os.environ.copy()
    wm_env["DISPLAY"] = TEST_DISP
    
    t0 = time.perf_counter()
    p_wm = subprocess.Popen(["perf", "stat", "-o", perf_out, bin_path],
                            env=wm_env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    
    time.sleep(0.3)
    t_startup = (time.perf_counter() - t0 - 0.3) * 1000  # ms estimate
    
    # Read memory metrics
    mem_stats = {}
    try:
        with open(f"/proc/{p_wm.pid}/status", "r") as f:
            for l in f:
                for k in ["VmRSS", "VmSize", "VmData", "VmStk", "VmExe"]:
                    if l.startswith(k + ":"):
                        mem_stats[k] = l.split()[1] + " " + l.split()[2]
    except Exception as e:
        mem_stats["error"] = str(e)
    
    # 4. Execute Standard Workload
    def xdo(*args):
        subprocess.run(["xdotool", *args], env=wm_env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(0.08)

    # Spawn 2 windows
    xdo("key", "alt+Return")
    xdo("key", "alt+w")
    # Horizontal split
    xdo("key", "alt+v")
    xdo("key", "alt+Return")
    # Spatial navigation
    xdo("key", "alt+j")
    xdo("key", "alt+k")
    xdo("key", "alt+h")
    xdo("key", "alt+l")
    # Spatial swap
    xdo("key", "alt+Shift+j")
    xdo("key", "alt+Shift+k")
    # Fullscreen toggle
    xdo("key", "alt+f")
    xdo("key", "alt+f")
    # Workspaces
    xdo("key", "alt+2")
    xdo("key", "alt+Return")
    xdo("key", "alt+1")
    # Move window to ws 5
    xdo("key", "alt+Shift+5")
    xdo("key", "alt+5")
    # Kill window
    xdo("key", "alt+q")
    
    # Exit WM cleanly
    xdo("key", "alt+Shift+q")
    time.sleep(0.2)
    
    if p_wm.poll() is None:
        p_wm.terminate()
        try:
            p_wm.wait(timeout=1.0)
        except:
            p_wm.kill()
    
    p_xephyr.terminate()
    p_parent.terminate()
    cleanup()
    
    # Parse perf output
    perf_data = {}
    if os.path.exists(perf_out):
        with open(perf_out, "r") as f:
            content = f.read()
            for line in content.splitlines():
                if "cycles:u" in line or "cycles" in line:
                    m = re.search(r"([\d\.,]+)\s+cycles", line)
                    if m: perf_data["cycles"] = m.group(1).replace(".", "").replace(",", "")
                if "instructions:u" in line or "instructions" in line:
                    m = re.search(r"([\d\.,]+)\s+instructions", line)
                    if m: perf_data["instructions"] = m.group(1).replace(".", "").replace(",", "")
                    m_ipc = re.search(r"#\s+([\d\.,]+)\s+insn per cycle", line)
                    if m_ipc: perf_data["ipc"] = m_ipc.group(1)
                if "branches:u" in line or "branches" in line:
                    m = re.search(r"([\d\.,]+)\s+branches", line)
                    if m: perf_data["branches"] = m.group(1).replace(".", "").replace(",", "")
                if "branch-misses:u" in line or "branch-misses" in line:
                    m = re.search(r"([\d\.,]+)\s+branch-misses", line)
                    if m: perf_data["branch_misses"] = m.group(1).replace(".", "").replace(",", "")
                    m_bm = re.search(r"#\s+([\d\.,]+)%\s+of all branches", line)
                    if m_bm: perf_data["branch_miss_rate"] = m_bm.group(1) + "%"
                if "page-faults:u" in line or "page-faults" in line:
                    m = re.search(r"([\d\.,]+)\s+page-faults", line)
                    if m: perf_data["page_faults"] = m.group(1).replace(".", "").replace(",", "")
                if "task-clock:u" in line or "task-clock" in line:
                    m = re.search(r"([\d\.,]+)\s+msec task-clock", line)
                    if m: perf_data["task_clock_ms"] = m.group(1)

    # Size data
    size_res = subprocess.run(["size", bin_path], capture_output=True, text=True)
    size_info = {}
    lines = size_res.stdout.strip().splitlines()
    if len(lines) >= 2:
        parts = lines[1].split()
        size_info = {
            "text": parts[0],
            "data": parts[1],
            "bss": parts[2],
            "total_dec": parts[3]
        }

    return {
        "name": name,
        "path": bin_path,
        "size": size_info,
        "mem": mem_stats,
        "perf": perf_data
    }

if __name__ == "__main__":
    results = {}
    for name, path in BINARIES.items():
        print(f"[*] Benchmarking {name}...")
        results[name] = run_benchmark_for_binary(name, path)
        print(f"    Done {name}.")
    
    with open(f"{LOG_DIR}/results.json", "w") as f:
        json.dump(results, f, indent=2)
    print("\n[+] All benchmarks completed successfully. Results saved.")
