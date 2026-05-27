#!/bin/bash
echo "🔍 TEST C585-B - HARDWARE & i915 VERIFICATION"
echo "═══════════════════════════════════════════════════════════════════════"
echo ""

echo "## 1. GPU HARDWARE"
echo "---"
lspci -nn | grep -i vga
echo ""

echo "## 2. GPU DEVICE INFO"
echo "---"
ls -la /dev/dri/
echo ""

echo "## 3. i915 MODULE INFO"
echo "---"
modinfo i915 | head -20
echo ""

echo "## 4. i915 PARAMETERS"
echo "---"
cat /sys/module/i915/parameters/enable_guc 2>/dev/null || echo "enable_guc: N/A"
cat /sys/module/i915/parameters/enable_fbc 2>/dev/null || echo "enable_fbc: N/A"
cat /sys/module/i915/parameters/enable_psr 2>/dev/null || echo "enable_psr: N/A"
echo ""

echo "## 5. GPU FREQUENCY"
echo "---"
cat /sys/class/drm/card0/gt_cur_freq_mhz 2>/dev/null || echo "freq: N/A"
cat /sys/class/drm/card0/gt_max_freq_mhz 2>/dev/null || echo "max_freq: N/A"
echo ""

echo "## 6. GPU ERROR STATE"
echo "---"
cat /sys/class/drm/card0/error 2>/dev/null | head -50 || echo "No errors"
echo ""

echo "## 7. DMESG i915 (last 50 lines)"
echo "---"
dmesg | grep -i i915 | tail -50
echo ""

echo "✅ Hardware check complete"
