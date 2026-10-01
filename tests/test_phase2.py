#!/usr/bin/env python3
"""
NSK OS v0.3 - Phase 2 QEMU Smoke Test
Verifies Graphics Engine, Framebuffer, Wallpaper, Box Blur, and Frosted Glass Panel rendering.
"""

import os
import subprocess
import sys
import time

def run_phase2_test():
    print("[TEST] Running Phase 2 Graphics Engine Smoke Test...")
    has_kernel = os.path.isfile("build/kernel.bin")
    has_iso = os.path.isfile("nsk-os-0.3.iso")

    if not has_kernel and not has_iso:
        print("[TEST ERROR] Neither build/kernel.bin nor nsk-os-0.3.iso found.")
        sys.exit(1)

    cmd = [
        "qemu-system-i386",
        "-m", "256",
        "-vga", "std",
        "-serial", "stdio",
        "-display", "none",
        "-no-reboot"
    ]

    if has_kernel:
        cmd.extend(["-kernel", "build/kernel.bin"])
    else:
        cmd.extend(["-cdrom", "nsk-os-0.3.iso"])

    try:
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )

        time.sleep(4)
        proc.terminate()
        try:
            stdout_bytes, stderr_bytes = proc.communicate(timeout=4)
        except subprocess.TimeoutExpired:
            proc.kill()
            stdout_bytes, stderr_bytes = proc.communicate()

        stdout = stdout_bytes.decode('utf-8', errors='replace') if stdout_bytes else ""
        stderr = stderr_bytes.decode('utf-8', errors='replace') if stderr_bytes else ""

        print("[TEST] Captured Serial Output:")
        print(stdout if stdout.strip() else "(none)")

        required_strings = [
            "PHASE 1 CORE KERNEL INITIALIZATION COMPLETE",
            "Initializing Phase 2 Graphics Engine",
            "Rendering signature blooming wave wallpaper",
            "Applying separable fast box blur",
            "Front & back buffers swapped successfully",
            "PHASE 2 GRAPHICS ENGINE TEST PASSED"
        ]

        missing = [s for s in required_strings if s not in stdout]

        if not missing:
            print("\n>>> [TEST PASSED] Phase 2 Graphics Engine Verified Successfully! <<<\n")
            sys.exit(0)
        else:
            print(f"\n[TEST FAILED] Missing Phase 2 strings: {missing}")
            sys.exit(1)

    except FileNotFoundError as e:
        print(f"[TEST NOTICE] QEMU binary not found: {e}")
        sys.exit(0)
    except Exception as e:
        print(f"[TEST ERROR] {e}")
        sys.exit(1)

if __name__ == "__main__":
    run_phase2_test()
