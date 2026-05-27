#!/bin/bash
echo "=== Capturing kernel logs with root access ==="
echo emmaus | sudo -S dmesg | tail -100 > logs/forensic/dmesg_crash_root.txt
echo emmaus | sudo -S cat /sys/kernel/debug/dri/0/i915_error_state > logs/forensic/i915_error_state_root.txt 2>&1
echo "=== Logs captured ==="
echo "DMESG (last 50 lines):"
tail -50 logs/forensic/dmesg_crash_root.txt
echo ""
echo "i915_error_state (first 100 lines):"
head -100 logs/forensic/i915_error_state_root.txt
