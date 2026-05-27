#!/bin/bash
# C331 - Traçage nanoseconde complet de l'exécution

set -e

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="logs/c331_trace_${TIMESTAMP}"
mkdir -p "${LOG_DIR}"

echo "═══════════════════════════════════════════════════════════════"
echo "  C331 - TRAÇAGE NANOSECONDE COMPLET"
echo "═══════════════════════════════════════════════════════════════"
echo "Timestamp: ${TIMESTAMP}"
echo "Log directory: ${LOG_DIR}"
echo ""

# Activer debug i915 maximal
echo "Activation debug i915 DRM (0xffffffff)..."
sudo sh -c 'echo 0xffffffff > /sys/module/drm/parameters/debug' 2>/dev/null || true
sudo sh -c 'echo 0xffffffff > /sys/module/i915/parameters/debug_dri' 2>/dev/null || true

# Compiler avec tous les symboles de debug
echo "Compilation avec debug complet..."
make clean > /dev/null 2>&1
make CFLAGS="-g3 -O0 -DDEBUG_TRACE_ALL" > "${LOG_DIR}/compile.log" 2>&1

# Capturer état système AVANT
echo "Capture état système AVANT..."
{
    echo "=== TIMESTAMP AVANT ==="
    date +%s.%N
    echo ""
    echo "=== GPU INFO ==="
    lspci -v -s 00:02.0 2>/dev/null || echo "GPU info unavailable"
    echo ""
    echo "=== MEMORY INFO ==="
    free -h
    echo ""
    echo "=== DRM DEVICES ==="
    ls -la /dev/dri/
    echo ""
    echo "=== I915 PARAMS ==="
    cat /sys/module/i915/parameters/* 2>/dev/null | head -50
} > "${LOG_DIR}/system_before.log"

# Lancer exécution avec strace + perf
echo "Lancement exécution avec traçage complet (10 secondes max)..."
timeout 10s strace -f -tt -T -o "${LOG_DIR}/strace.log" \
    perf record -e 'i915:*' -e 'drm:*' -o "${LOG_DIR}/perf.data" \
    ./tests/test_btc_mining_native > "${LOG_DIR}/stdout.log" 2> "${LOG_DIR}/stderr.log" || true

# Capturer état système APRÈS
echo "Capture état système APRÈS..."
{
    echo "=== TIMESTAMP APRÈS ==="
    date +%s.%N
    echo ""
    echo "=== DMESG (dernières 200 lignes) ==="
    dmesg | tail -200
    echo ""
    echo "=== MEMORY INFO ==="
    free -h
} > "${LOG_DIR}/system_after.log"

# Copier tous les logs forensiques
echo "Copie logs forensiques..."
cp -r logs/forensic/* "${LOG_DIR}/" 2>/dev/null || true
cp -r logs/metrics/* "${LOG_DIR}/" 2>/dev/null || true
cp -r logs/hardware/* "${LOG_DIR}/" 2>/dev/null || true

# Générer rapport perf
echo "Génération rapport perf..."
perf script -i "${LOG_DIR}/perf.data" > "${LOG_DIR}/perf_script.log" 2>/dev/null || true

# Analyser strace pour identifier les appels critiques
echo "Analyse strace..."
{
    echo "=== IOCTL DRM_IOCTL_I915_GEM_EXECBUFFER2 ==="
    grep "DRM_IOCTL_I915_GEM_EXECBUFFER2" "${LOG_DIR}/strace.log" || echo "Aucun trouvé"
    echo ""
    echo "=== IOCTL DRM_IOCTL_I915_GEM_SET_DOMAIN ==="
    grep "DRM_IOCTL_I915_GEM_SET_DOMAIN" "${LOG_DIR}/strace.log" || echo "Aucun trouvé"
    echo ""
    echo "=== IOCTL DRM_IOCTL_I915_GEM_WAIT ==="
    grep "DRM_IOCTL_I915_GEM_WAIT" "${LOG_DIR}/strace.log" || echo "Aucun trouvé"
    echo ""
    echo "=== MMAP ==="
    grep "mmap" "${LOG_DIR}/strace.log" | head -20 || echo "Aucun trouvé"
} > "${LOG_DIR}/strace_analysis.log"

# Résumé
echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  TRAÇAGE TERMINÉ"
echo "═══════════════════════════════════════════════════════════════"
echo "Tous les logs sont dans: ${LOG_DIR}/"
echo ""
echo "Fichiers générés:"
ls -lh "${LOG_DIR}/"
echo ""
echo "Pour analyser:"
echo "  - Logs forensiques: ${LOG_DIR}/btc_mining_native_sha256.log"
echo "  - Stdout: ${LOG_DIR}/stdout.log"
echo "  - Stderr: ${LOG_DIR}/stderr.log"
echo "  - Strace: ${LOG_DIR}/strace.log"
echo "  - Perf: ${LOG_DIR}/perf_script.log"
echo "  - Système avant: ${LOG_DIR}/system_before.log"
echo "  - Système après: ${LOG_DIR}/system_after.log"
echo ""
