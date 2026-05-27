#!/bin/bash
# TEST C435 - TRACE GPU FORENSIQUE BIT-LEVEL 100%
# Objectif: Capturer TOUS les événements GPU nanoseconde par nanoseconde

set -e

cd "$(dirname "$0")"

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="logs/forensic"
mkdir -p "$LOG_DIR"

echo "🚀 TEST C435 - ACTIVATION LOGGING FORENSIQUE BIT-LEVEL 100%"
echo "📊 Timestamp: $TIMESTAMP"

# 1. Activer tracing kernel i915
echo "⚙️  Activation tracing i915 DRM..."
echo emmaus | sudo -S sh -c "echo 1 > /sys/kernel/debug/tracing/events/i915/enable" 2>/dev/null || true
echo emmaus | sudo -S sh -c "echo 1 > /sys/kernel/debug/tracing/tracing_on" 2>/dev/null || true

# 2. Lancer intel_gpu_top en background
echo "⚙️  Lancement intel_gpu_top..."
echo emmaus | sudo -S intel_gpu_top -o "$LOG_DIR/gpu_top_c435_$TIMESTAMP.log" -s 100 &
GPU_TOP_PID=$!
sleep 1

# 3. Capturer état GPU avant exécution
echo "📸 Capture état GPU AVANT..."
cat /sys/kernel/debug/dri/0/i915_engine_info > "$LOG_DIR/gpu_engine_before_c435_$TIMESTAMP.log" 2>/dev/null || true
cat /sys/kernel/debug/dri/0/i915_gem_objects > "$LOG_DIR/gpu_objects_before_c435_$TIMESTAMP.log" 2>/dev/null || true

# 4. Exécuter test avec trace
echo "⚡ Exécution test C433 avec binaire OpenCL..."
timeout 5s ./test_c433_dispatch_runner tests/test_c434_opencl_extracted.bin 2>&1 | tee "$LOG_DIR/execution_c435_$TIMESTAMP.log"
EXIT_CODE=$?

# 5. Capturer état GPU après exécution
echo "📸 Capture état GPU APRÈS..."
cat /sys/kernel/debug/dri/0/i915_engine_info > "$LOG_DIR/gpu_engine_after_c435_$TIMESTAMP.log" 2>/dev/null || true
cat /sys/kernel/debug/dri/0/i915_gem_objects > "$LOG_DIR/gpu_objects_after_c435_$TIMESTAMP.log" 2>/dev/null || true

# 6. Capturer trace i915
echo "📝 Capture trace i915..."
echo emmaus | sudo -S cat /sys/kernel/debug/tracing/trace > "$LOG_DIR/i915_trace_c435_$TIMESTAMP.log" 2>/dev/null || true

# 7. Arrêter intel_gpu_top
sleep 2
echo emmaus | sudo -S kill $GPU_TOP_PID 2>/dev/null || true

# 8. Désactiver tracing
echo emmaus | sudo -S sh -c "echo 0 > /sys/kernel/debug/tracing/tracing_on" 2>/dev/null || true
echo emmaus | sudo -S sh -c "echo 0 > /sys/kernel/debug/tracing/events/i915/enable" 2>/dev/null || true

echo ""
echo "✅ TEST C435 TERMINÉ"
echo "📊 Logs générés:"
echo "   - $LOG_DIR/execution_c435_$TIMESTAMP.log"
echo "   - $LOG_DIR/i915_trace_c435_$TIMESTAMP.log"
echo "   - $LOG_DIR/gpu_top_c435_$TIMESTAMP.log"
echo "   - $LOG_DIR/gpu_engine_before_c435_$TIMESTAMP.log"
echo "   - $LOG_DIR/gpu_engine_after_c435_$TIMESTAMP.log"
echo "   - $LOG_DIR/gpu_objects_before_c435_$TIMESTAMP.log"
echo "   - $LOG_DIR/gpu_objects_after_c435_$TIMESTAMP.log"
echo ""
echo "🔍 Prochaine étape: Analyser les traces pour identifier où le GPU produit des zéros"

exit $EXIT_CODE

# Made with Bob
