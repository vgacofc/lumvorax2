#!/bin/bash
# CYCLE C197.10 - Capture GEM objects pendant exécution OpenCL

echo "=== C197.10 - CAPTURE GEM OBJECTS LIVE ==="
echo ""

# Mot de passe sudo
SUDO_PASS="emmaus"

# Étape 1 : Capturer état AVANT
echo "[STEP 1] Capturing GEM objects BEFORE OpenCL execution..."
echo "$SUDO_PASS" | sudo -S cat /sys/kernel/debug/dri/0000:00:02.0/i915_gem_objects > c197_10_gem_before.txt 2>&1
echo "  Saved: c197_10_gem_before.txt"
echo ""

# Étape 2 : Lancer kernel OpenCL en arrière-plan
echo "[STEP 2] Launching OpenCL kernel..."
./test_c197_10_capture_command_stream > /dev/null 2>&1 &
OPENCL_PID=$!
echo "  OpenCL PID: $OPENCL_PID"
echo ""

# Étape 3 : Attendre 100ms pour que le kernel démarre
echo "[STEP 3] Waiting for kernel to start (100ms)..."
sleep 0.1
echo ""

# Étape 4 : Capturer état PENDANT (si processus encore actif)
if ps -p $OPENCL_PID > /dev/null 2>&1; then
    echo "[STEP 4] Capturing GEM objects DURING OpenCL execution..."
    echo "$SUDO_PASS" | sudo -S cat /sys/kernel/debug/dri/0000:00:02.0/i915_gem_objects > c197_10_gem_during.txt 2>&1
    echo "  Saved: c197_10_gem_during.txt"
else
    echo "[STEP 4] OpenCL already finished (too fast)"
fi
echo ""

# Étape 5 : Attendre fin du processus
echo "[STEP 5] Waiting for OpenCL to complete..."
wait $OPENCL_PID
OPENCL_EXIT=$?
echo "  OpenCL exit code: $OPENCL_EXIT"
echo ""

# Étape 6 : Capturer état APRÈS
echo "[STEP 6] Capturing GEM objects AFTER OpenCL execution..."
echo "$SUDO_PASS" | sudo -S cat /sys/kernel/debug/dri/0000:00:02.0/i915_gem_objects > c197_10_gem_after.txt 2>&1
echo "  Saved: c197_10_gem_after.txt"
echo ""

# Étape 7 : Analyser les différences
echo "[STEP 7] Analyzing differences..."
echo ""

echo "=== GEM OBJECTS BEFORE ==="
wc -l c197_10_gem_before.txt
echo ""

if [ -f c197_10_gem_during.txt ]; then
    echo "=== GEM OBJECTS DURING ==="
    wc -l c197_10_gem_during.txt
    echo ""
fi

echo "=== GEM OBJECTS AFTER ==="
wc -l c197_10_gem_after.txt
echo ""

echo "=== RÉSULTATS C197.10 ==="
echo "✓ Capture complete"
echo "Files generated:"
echo "  - c197_10_gem_before.txt"
if [ -f c197_10_gem_during.txt ]; then
    echo "  - c197_10_gem_during.txt"
fi
echo "  - c197_10_gem_after.txt"
echo ""
echo "Next: Analyze GEM objects to identify OpenCL buffers"

# Made with Bob
